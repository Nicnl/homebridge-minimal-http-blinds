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

String pers; // %% from Homebridge
int w_state = 0; // "0" close, "1" open, "2" idle

void handleRoot(); // Hellow World) Classic !!!
void handleLogin(); // POST Homebridge Plugin http-minimal-blinds
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

// If need Serial port
// Serial.begin(115200);

WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password); // Connect to WiFi
// Serial.println("");
while (WiFi.status() != WL_CONNECTED) // Waiting connect
{
delay(250);
// Serial.print(".");
}

ArduinoOTA.setPort(8267); // Port OTA
ArduinoOTA.setHostname("WeMos-klw"); // Host name
// ArduinoOTA.setPassword((const char *)"123");
ArduinoOTA.onStart([]() {
//Serial.println("Start"); 
});
ArduinoOTA.onEnd([]() {
//Serial.println("\nEnd"); 
});
ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
});
ArduinoOTA.onError([](ota_error_t error) {
//Serial.printf("Error[%u]: ", error);
if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed"); 
else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed"); 
else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed"); 
else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed"); 
else if (error == OTA_END_ERROR) Serial.println("End Failed"); 
});
ArduinoOTA.begin();
//Serial.println("Ready");
//Serial.print("IP address: ");
//Serial.println(WiFi.localIP());
blink();
blink();

//WebSerwer responds
//server.on("/side", HTTP_GET, handleState); // Side of stepper motor
server.on("/state", HTTP_GET, handleState); // State
server.on("/position", HTTP_GET, handlePosition); // Position 0 - 100
server.on("/", HTTP_GET, handleRoot); // Hellow World)
server.on("/set", HTTP_POST, handleLogin); //Position from Homebridge
server.onNotFound(handleNotFound); //Wrong request
server.begin();
//Serial.println("HTTP server started");
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
pers = "100";
w_state = 1;
int persent = pers.toInt();
stepper.moveTo(persent * 192);
stepper.run();
}
if (value == 8983010) // RF signal for close
{
pers = "0";
w_state = 0;
int persent = pers.toInt();
stepper.moveTo(persent * 192);
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
void handleLogin()
{
pers = server.arg("pers"); // Get persentage of position string
int persent = pers.toInt();
if ( persent >= 0 && persent <= 100 )
{
w_state = 2;
stepper.moveTo(persent * 192);
stepper.run();
// Serial.print("Position :" + pers);
server.send(204, "text/plain", server.arg("set"));
}
else
{
server.send(401, "text/plain", "401: Wrong Position");
}
}
void handlePosition()
{
stepper.run();
server.send(200, "text/plain", String(pers)); //Current position 0 - 100
}
void handleState()
{
stepper.run();
if (pers == "0") {
w_state = 0;
}
if (pers == "100") {
w_state = 1;
}
if (pers != "0" && pers != "100") {
w_state = 2;
}
//String respMsg = "State is Closing \n";
server.send(200, "text/plain", String(w_state)); // Current state "0" close, "1" open, "2" idle
//Serial.println(w_state);
}
void handleNotFound()
{
stepper.run();
server.send(404, "text/plain", "404: Not found");
}
