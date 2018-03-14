# homebridge-minimal-http-blinds

### What is it?

**homebridge-minimal-http-blinds** is a minimalistic HTTP blinds or roller shutters management plugin for homebridge.

The features:
- You can control your own blinds/roller shutters apparatus with three minimalistic HTTP requests.
- The control is not a simple binary open/close: **it support percentages**. You can open your blinds at 50% or 65% for instance.
- Your blinds can still be manually operated. As long at the `get_current_position_url` returns the right value, this plugin will update iOS Home app in real time.

### Who is it for?

Anyone who, just like me, don't know much about homebridge
but still want a straightforward way to communicate with your own home-made Raspberry Pi blinds/roller shutters controller.

### How to use it

#### 1] Install it into your homebridge instance

The installation instructions differs depending on how you installed homebridge.

Usually, it's something like _"add this to your homebridge's `install.sh`"_
````bash
npm install -g homebridge-minimal-http-blinds
````

#### 2] Minimal configuration

Here is an homebridge's `config.json` with the minimal valid configuration:

````json
{
    "bridge": {
        "name": "DemoMinimalisticHttpBlinds",
        "username": "AA:BB:CC:DD:EE:FF",
        "port": 51826,
        "pin": "123-45-678"
    },
  
    "description": "DEV NODEJS MACBOOK",
  
    "accessories": [
        {
            "name": "Kitchen Blinds",
            "accessory": "MinimalisticHttpBlinds",
  
            "get_current_position_url": "http://192.168.1.55/get/current_position/",
            "set_target_position_url": "http://192.168.1.55/set/%position%",
            "get_current_state_url": "http://192.168.1.55/get/current_state/"
        }
  
    ],
  
    "platforms": []
}
````

Beware, I'm a lazy ass!  
These three parameters are not checked!  
(`get_current_position_url`, `set_target_position_url`, `get_current_state_url`)  
If you forgot to write them in your accessory, the module will crash.

Also, in the `set_target_position_url` parameter, the placeholder `%position%` will be replaced by the value selected in the iPhone's Home App. 

#### 3] More configuration

There are more configuration options.  
The names are self-descriptive.  
Here are them all with their default values.

````
{
    "get_current_position_method": "GET",
    "set_target_position_method": "POST",
    "get_current_state_method": "GET",
    
    "get_current_position_expected_response_code": "200",
    "set_target_position_expected_response_code": "204",
    "get_current_state_expected_response_code": "200",
    
    "get_current_position_polling_millis": "500",
    "get_current_state_polling_millis": "500"
}
````

#### 4] Protocol requirements

The three URLs specified in the accessory configuration must have the following data formats:

##### 4.1] `get_current_position_url`

This URL must return the current blinds position **in plaintext**, from 0 to 100.  
(0 being closed and 100 opened)

##### 4.2] `set_target_position_url`

This URL must trigger the blinds movement.  
(That's the part you've done with your Raspberry Pi)  
The requested opening position is, once again, an integer from 0 to 100.
Please note that is passed **directly in the URL**. (It's the `%position%` placeholder)  

Yep, that's it.  
Not a single trace of json.  
Are we barbarians or are we not?    


##### 4.3] `get_current_state_url`

This is the trickiest URL.  
It must return one of these three integers **in plaintext**:
- 0 if the blinds are closing
- 1 if the blinds are opening
- 2 if the blinds are idling

Once again, that's your part of the job, the one you're supposed to implement in your blind manager thingymagic.

________________________________________

[Click here](EXAMPLE.MD) to see an example implementation of this HTTP server.

# That's all

## Enjoy
