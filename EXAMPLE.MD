# Example implementation in Python

## 1] Architecture

- Homebridge server is : `192.168.1.22`
- Raspberry Pi controlling the blinds : `192.168.1.55`

## 1] Homebridge accessory configuration

Here is the accessory I need to add:

``` json
"accessories": [
    {
        "name": "Kitchen Blinds",
        "accessory": "MinimalisticHttpBlinds",

        "get_current_position_url": "http://192.168.1.55:8080/get/current_position/",
        "set_target_position_url": "http://192.168.1.55:8080/set/%position%",
        "get_current_state_url": "http://192.168.1.55:8080/get/current_state/"
    }

]
```

Here is a reminder of the default values the accessory use if you don't write them explicitely:
```json
{
    "get_current_position_method": "GET",
    "set_target_position_method": "POST",
    "get_current_state_method": "GET",
    
    "get_current_position_expected_response_code": "200",
    "set_target_position_expected_response_code": "204",
    "get_current_state_expected_response_code": "200",
}
```

## 2] What the Homebridge server expects

As you can see, the Raspberry Pi will need to host an HTTP server providing three routes:
- `GET /get/current_position/` that responds `200 OK` with the current position of the blinds to Homebridge
- `POST /set/%position%/` that responds `204 NO CONTENT` to Homebridge
- `GET /get/current_state/` that responds `200 OK` with the current state of the blinds _(reminder: 0 if the blinds are closing, 1 if opening, 2 if idle)_

## 3] Example implementation in python

This example implementation uses Flask, a very simple and easy to use HTTP serving API. 

```python
from flask import Flask
app = Flask(__name__)


@app.route('/set/<position>', methods=['POST'])
def set_target_position(position):
    print('This code should physically set the blinds to ' + str(position) + '%')

    return '', 204


@app.route('/get/current_position/', methods=['GET'])
def get_current_position():
    current_position = 100
    print('This code should physically get the blinds position')

    return str(current_position), 200


@app.route('/get/current_state/', methods=['GET'])
def get_current_state():
    # IDLE/STOPPED       => 2
    # OPENING/INCREASING => 1
    # CLOSING/DECREASING => 0

    print('This code should physically find a way to know if the blinds are opening, closing or idling')
    
    if 'your blinds are not moving':
        return '2', 200
    elif 'your blinds are opening/increasing':
        return '1', 200
    elif 'your blinds are closing/decreasing':
        return '0', 200
    else:
        return '', 500  # wtf that should not happen

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
```
