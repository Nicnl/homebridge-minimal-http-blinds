var request = require('request');
var Service, Characteristic;

module.exports = function(homebridge) {
    Service = homebridge.hap.Service;
    Characteristic = homebridge.hap.Characteristic;
    homebridge.registerAccessory('homebridge-minimal-http-blinds', 'MinimalisticHttpBlinds', MinimalisticHttpBlinds);
    console.log('Loading MinimalisticHttpBlinds accessories...');
};

function MinimalisticHttpBlinds(log, config) {
    this.log = log;

    // Required parameters
    this.get_current_position_url = config.get_current_position_url;
    this.set_target_position_url  = config.set_target_position_url;
    this.get_current_state_url    = config.get_current_state_url;

    // Optional parameters: HTTP methods
    this.get_current_position_method = config.get_current_position_method || 'GET';
    this.set_target_position_method  = config.set_target_position_method  || 'POST';
    this.get_current_state_method    = config.get_current_state_method    || 'GET';

    // Optional parameters: expected HTTP response codes
    this.get_current_position_expected_response_code = parseInt(config.get_current_position_expected_response_code) || 200;
    this.set_target_position_expected_response_code  = parseInt(config.set_target_position_expected_response_code)  || 204;
    this.get_current_state_expected_response_code    = parseInt(config.get_current_state_expected_response_code)    || 200;

    // Optional parameters: polling times
    this.get_current_position_polling_millis = parseInt(config.get_current_position_polling_millis) || 500;
    this.get_current_state_polling_millis    = parseInt(config.get_current_state_polling_millis)    || 500;
    this.no_cache_duration_millis = parseInt(config.no_cache_duration_millis)    || (1000 * 60);


    // Internal fields
    this.current_position = undefined;
    this.current_state = undefined;
    this.current_position_error = new Error("Uninitialized");
    this.current_state_error = new Error("Uninitialized");


    this.get_current_position_callbacks = [];
    this.get_target_position_callbacks = [];
    this.get_current_state_callbacks = [];

    this.restart_cache_timer();

    // Initializing things
    setInterval(this.update_current_position.bind(this), this.get_current_position_polling_millis);
    setInterval(this.update_current_state.bind(this), this.get_current_state_polling_millis);

    this.init_service();
}

MinimalisticHttpBlinds.prototype.init_service = function() {
    this.service = new Service.WindowCovering(this.name);

    this.service.getCharacteristic(Characteristic.CurrentPosition).on('get', function(callback) {
        this.get_current_position_callbacks.push(callback);
        if(!this.cache_timer_active()) {
            this.log("completing get_current_position from cache");
            this.complete_get_current_position_callbacks(this.current_position, this.current_position_error);
        }
    }.bind(this));

    this.service.getCharacteristic(Characteristic.TargetPosition).on('get', function(callback) {
        this.get_target_position_callbacks.push(callback);
        if(!this.cache_timer_active()) {
            this.log("completing complete_get_target_position_callbacks from cache");
            this.complete_get_target_position_callbacks(this.current_state, this.current_state_error);
        }
    }.bind(this));
    this.service.getCharacteristic(Characteristic.TargetPosition).on('set', this.set_target_position.bind(this));

    // Note: iOS's Home App subtracts CurrentPosition from TargetPosition to determine if it's opening, closing or idle.
    // It absolutely doesn't care about Characteristic.PositionState, which is supposed to be :
    // PositionState.INCREASING = 1, PositionState.DECREASING = 0 or PositionState.STOPPED = 2
    // But in any case, let's still implement it
    this.service.getCharacteristic(Characteristic.PositionState).on('get', function() {
        this.get_current_state_callbacks.push(callback);
        if(!this.cache_timer_active()) {
            this.log("completing complete_get_current_state_callbacks from cache");
            this.complete_get_current_state_callbacks(this.current_state, this.current_state_error);
        }
    }.bind(this));
};

MinimalisticHttpBlinds.prototype.update_current_position = function() {
    request({
        url: this.get_current_position_url,
        method: this.get_current_position_method,
        timeout: 5000
    }, function(error, response, body) {
        if (error) {
            this.log('Error when polling current position.');
            this.log(error);
            this.current_position_error = error;
            this.complete_get_current_position_callbacks(this.current_position, this.current_position_error);
            return;
        }
        else if (response.statusCode != this.get_current_position_expected_response_code) {
            this.log('Unexpected HTTP status code when polling current position. Got: ' + response.statusCode + ', expected:' + this.get_current_position_expected_response_code);
            this.current_position_error = new Error("Error when polling current position.");
            this.complete_get_current_position_callbacks(this.current_position, this.current_position_error);
            return;
        }

        var new_position = parseInt(body);


        if (this.get_current_position_callbacks.length > 0) {
            this.complete_get_current_position_callbacks(new_position, null);
        }
        else if (new_position !== this.current_position && !this.notify_ios_blinds_has_stopped) {
            this.service.getCharacteristic(Characteristic.CurrentPosition).setValue(new_position);
            this.log('Updated CurrentPosition to value ' + new_position);
        }

        if (this.notify_ios_blinds_has_stopped) {
            this.notify_ios_blinds_has_stopped = false;

            this.log('Updated CurrentPosition and TargetPosition to value ' + new_position);
            this.service.getCharacteristic(Characteristic.CurrentPosition).setValue(new_position);
            this.service.getCharacteristic(Characteristic.TargetPosition).setValue(new_position, null, {
                'plz_do_not_actually_move_the_blinds': true
            });
        }

        this.current_position = new_position;
        this.current_position_error = null;
    }.bind(this));
};

MinimalisticHttpBlinds.prototype.complete_get_current_position_callbacks = function(position, error) {

    if (this.get_current_position_callbacks.length > 0) {
        this.get_current_position_callbacks.forEach(function (callback) {
            if(error !== null){
                this.log('calling callback with error: ' + error);
                callback(error);
            }
            else{
                this.log('calling callback with position: ' + position);
                callback(null, position);
            }
        }.bind(this));
        this.log('Responded to ' + this.get_current_position_callbacks.length + ' CurrentPosition callbacks!');
        this.get_current_position_callbacks = [];
    }
}

MinimalisticHttpBlinds.prototype.complete_get_current_state_callbacks = function(state, error) {

    if (this.get_current_state_callbacks.length > 0) {
        this.get_current_state_callbacks.forEach(function (callback) {
            if(error !== null){
                callback(error);
            }
            else{
                callback(null, state);
            }
        }.bind(this));
        this.log('Responded to ' + this.get_current_state_callbacks.length + ' PositionState callbacks!');
        this.get_current_state_callbacks = [];
    }
}


MinimalisticHttpBlinds.prototype.complete_get_target_position_callbacks = function(state, error) {
    // This is ugly: we're faking the target position to either 0, 100 or the current position,
    // so that iOS's Home App displays the right state (opening, closing, idle)
    var target_position = this.current_position;
    if (state === 1) target_position = 100;
    else if (state === 0) target_position = 0;

    if (this.get_target_position_callbacks.length > 0) {
        this.get_target_position_callbacks.forEach(function (callback) {
            if(error !== null){
                callback(error);
            }
            else{
                callback(null, target_position);
            }
        }.bind(this));
        this.log('Responded to ' + this.get_target_position_callbacks.length + ' TargetPosition callbacks!');
        this.get_target_position_callbacks = [];
    }
}

MinimalisticHttpBlinds.prototype.update_current_state = function() {
    request({
        url: this.get_current_state_url,
        method: this.get_current_state_method,
        timeout: 5000
    }, function(error, response, body) {
        if (error) {
            this.log('Error when polling current state.');
            this.log(error);
            this.current_state_error = error;
            this.complete_get_current_state_callbacks(this.current_state, this.current_state_error);
            this.complete_get_target_position_callbacks(this.current_state, this.current_state_error);
            return;
        }
        else if (response.statusCode != this.get_current_state_expected_response_code) {
            this.log('Unexpected HTTP status code when polling current state. Got: ' + response.statusCode + ', expected:' + this.get_current_state_expected_response_code);
            this.current_state_error = new Error("Error polling state");
            this.complete_get_current_state_callbacks(this.current_state, this.current_state_error);
            this.complete_get_target_position_callbacks(this.current_state, this.current_state_error);
            return;
        }

        var new_state = parseInt(body);

        if (new_state !== this.current_state && new_state === 2)
            this.notify_ios_blinds_has_stopped = true;

        if (this.get_current_state_callbacks.length > 0) {
            this.complete_get_current_state_callbacks(new_state);
        }
        else if (new_state !== this.current_state) {
            // Sooo, yeah... We're updating PositionState, but iOS doesn't care anyway... we still do it for the lolz.
            this.service.getCharacteristic(Characteristic.PositionState).setValue(new_state);
            this.log('Updated PositionState to value ' + new_state);
        }

        this.current_state = new_state;
        this.current_state_error = null;

        this.complete_get_target_position_callbacks(this.current_state, this.current_state_error);
    }.bind(this));
};

MinimalisticHttpBlinds.prototype.stop_cache_timer = function() {
    if(this.cache_timer_active())
    {
        clearTimeout(this.stop_using_cache_timer);
    }
    this.stop_using_cache_timer = null;
}
MinimalisticHttpBlinds.prototype.cache_timer_active = function() {
    return this.stop_using_cache_timer !== null;
}

MinimalisticHttpBlinds.prototype.restart_cache_timer = function() {
    this.stop_cache_timer();
    this.stop_using_cache_timer = setTimeout(this.stop_cache_timer.bind(this), this.no_cache_duration_millis);
}

MinimalisticHttpBlinds.prototype.set_target_position = function(position, callback, context) {

    this.restart_cache_timer();

    if (context && context.plz_do_not_actually_move_the_blinds) {
        this.log('set_target_position is ignoring an actual request...');
        callback(new Error('set_target_position is ignoring an actual request...'));
        return;
    }

    this.log('Setting new target position: ' + position + ' => ' + this.set_target_position_url.replace('%position%', position));
    request({
        url: this.set_target_position_url.replace('%position%', position),
        method: this.set_target_position_method
    }, function(error, response, body) {
        if (error || response.statusCode != this.set_target_position_expected_response_code) {
            this.log('Error when setting new target position: ' + body);
            callback(error || new Error("Error updating shade position."));
            return;
        }
        this.log('Target position set to ' + position);
        callback(null)
    }.bind(this));
};

MinimalisticHttpBlinds.prototype.getServices = function() {
    return [this.service];
};