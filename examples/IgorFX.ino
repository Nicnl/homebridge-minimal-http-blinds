/* config.json file for homebridge-minimal-http-blinds
"get_current_position_url": "http://192.168.0.100/position",
"set_target_position_url": "http://192.168.0.100/set?persent=%position%",
"get_current_state_url": "http://192.168.0.100/state"
*/

#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <AccelStepper.h> //Stepper motor super library :p
#include <ESP8266mDNS.h> 
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RCSwitch.h> //RF433 library
#define HALFSTEP 8
}

// Stepper pins setup
#define Pin1 14 // IN1 ULN2003 
#define Pin2 12 // IN2 ULN2003
#define Pin3 13 // IN3 ULN2003
#define Pin4 15 // IN4 ULN2003

//  IN1-IN3-IN2-IN4 AccelStepper with stepper motor driver
AccelStepper stepper(HALFSTEP, Pin1, Pin3, Pin2, Pin4);

// RF433 setup
RCSwitch mySwitch = RCSwitch();

// WiFi Connection setup
const char* ssid = "ssidwifipoint"; // SSID WiFi
const char* password = "passwifipoint"; // Passwaord WiFi
const char* host = "WeMos-klw"; // BasicOTA host name
IPAddress ip(192, 168, 0, 100); // Static IP
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(80); // WebServer Port

String persent; // %% from Homebridge
int blinds_state = 0; // "0" close, "1" open, "2" idle

void handleRoot(); // Hellow World) Classic !!!
void handleSet(); // POST Homebridge Plugin http-minimal-blinds
void handleNotFound(); // Wrong request
void handleState(); // Homebridge state update 0,1,2
void handlePosition(); // Homebridge position update

void blink() // Blink for same action
{
digitalWrite(LED_BUILTIN, LOW);
delay(500);
digitalWrite(LED_BUILTIN, HIGH);
delay(500);
}

void setup(void)
{
stepper.setMaxSpeed(500); // Max speed stepper motor
stepper.setAcceleration(250); // Acceleration stepper motor
stepper.setSpeed(250); // steps per second
stepper.setCurrentPosition(0); // Zero Point...If module crash we here
mySwitch.enableReceive(D1);

// GPIO setup
pinMode(LED_BUILTIN, OUTPUT);
digitalWrite(LED_BUILTIN, HIGH);

// WiFi Connection
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password); // Connect to WiFi
while (WiFi.status() != WL_CONNECTED) // Waiting connect
{
delay(250);
}

ArduinoOTA.setPort(8267); // Port OTA
ArduinoOTA.setHostname("WeMos-klw"); // Host name
ArduinoOTA.onStart([]() {
});
ArduinoOTA.onEnd([]() {
});
ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
});
ArduinoOTA.onError([](ota_error_t error) {

if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed"); 
else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed"); 
else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed"); 
else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed"); 
else if (error == OTA_END_ERROR) Serial.println("End Failed"); 
});
ArduinoOTA.begin();

blink();
blink();

//WebSerwer responds
//server.on("/side", HTTP_GET, handleState); // Side of stepper motor
server.on("/state", HTTP_GET, handleState); // State
server.on("/position", HTTP_GET, handlePosition); // Position 0 - 100
server.on("/", HTTP_GET, handleRoot); // Hellow World)
server.on("/set", HTTP_POST, handleSet); //Position from Homebridge
server.onNotFound(handleNotFound); //Wrong request
server.begin();

}

void loop(void)
{
ArduinoOTA.handle();
stepper.run();
server.handleClient();

if (mySwitch.available()) 
{
int value = mySwitch.getReceivedValue(); // RF signal to full open
if (value == 8983009)
{
persent = "100";
blinds_state = 1;
int persents = persent.toInt();
stepper.moveTo(persents * 192);
stepper.run();
}
if (value == 8983010) // RF signal for close
{
persent = "0";
blinds_state = 0;
int persents = persent.toInt();
stepper.moveTo(persents * 192);
stepper.run();
}
mySwitch.resetAvailable();
}

if (!stepper.isRunning()) // EnergySaving !!!
{
stepper.disableOutputs();
}
}
void handleRoot()
{
stepper.run();
server.send(200, "text/plain", "Hello World");
}
void handleSet()
{
persents = server.arg("persent"); // Get persentage of position string
int persents = persent.toInt();
if ( persents >= 0 && persents <= 100 )
{
blinds_state = 2;
stepper.moveTo(persents * 192);
stepper.run();
server.send(204, "text/plain");
}
else
{
server.send(401, "text/plain", "401: Wrong Position");
}
}
void handlePosition()
{
stepper.run();
server.send(200, "text/plain", String(persents)); //Current position 0 - 100
}
void handleState()
{
stepper.run();
if (persent == "0") {
blinds_state = 0;
}
if (persent == "100") {
blinds_state = 1;
}
if (persent != "0" && persent != "100") {
blinds_state = 2;
}
server.send(200, "text/plain", String(blinds_state)); // Current state "0" close, "1" open, "2" idle
}
void handleNotFound()
{
stepper.run();
server.send(404, "text/plain", "404: Not found");
}
