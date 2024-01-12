# Smart-Street-Lighting-System
## Summary
The goal of the project is to prove that with the help of various sensors,microcontroller and software,we can use energy minimally using Pulse Width Modulation(PWM),such that only a certain amount of light energy is used when needed;thus saving and ensuring efficiency for street lighting.
## LOGIC
A Smart Street Lighting System would,as the name suggests, try to work with little or no human intervention,and mine is no different.3 key sensors implement that.
1. RTC and Light sensors ensure a correct time for the lights to go on,taking into consideration winter,summer times.
1. Temperature & Humidity sensors ensure the correct conditions are met to regulate lighting i.e; misty conditions would trigger maximum brightness.
1. Proximity sensors(PIR,ULtrasonic) ensure maximum brightness is utilized especially when objects are at predetermined distances at the vicinity.
   
A combination of these 3 form a robust system,but unfortunately for this project,I implemented each separately,although it does not affect the general outcome negatively.
## Components
1. Sensors-PIR/Ultrasonic,Light,Current,Voltage ,Temperature and Humidity 
1. MCUs-Arduino R3 and ESP8266
1. Power supply-battery and/or AC supply
1. Breadboards
1. Load-LEDs,Resistors etc
1. Real Time Clock
1. Software-Arduino IDE,python,MQTT broker and client app.
## Block Diagram
![image](https://github.com/Donnybroke/smart_street_lighting/assets/92339420/dd358934-1703-4ef7-a38d-70352f04fbc3)
## Schematic Diagram
Using KICAD 7.0, the functional schematic diagram was plotted,showing all functional sections and components.
![image](https://github.com/Donnybroke/smart_street_lighting/assets/92339420/60367e7e-fd8d-4090-827b-a3bf2323f7ae)
## PCB
![image](https://github.com/Donnybroke/smart_street_lighting/assets/92339420/eaeb3163-3d15-44e5-b485-fe7d8e171ddd)
## Arduino Code
The software part of the project includes the arduino,ESP8266 node MCU and MQTT.
+ Importing and Declarations of Libraries and constants/variables
  
```
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include "DHTesp.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_INA219.h>
const int  bluePin=14,redPin=15,DHTpin=12,ldrPin=A0,ldrStatus,greenPin=13;
DHTesp dht;
Adafruit_INA219 ina219;

//code to run ultrasonic sensor
const int trigPin = 2;  // Trigger pin of the ultrasonic sensor
const int echoPin = 0; // Echo pin of the ultrasonic sensor

/****** WiFi Connection Details *******/
const char* ssid = "xxxxxxxx";
const char* password = "xxxxxxxx";

/******* MQTT Broker Connection Details *******/
const char* mqtt_server = "xxxxxxxx";
const char* mqtt_username = "xxxxxxxx";
const char* mqtt_password = "xxxxxxxx";
const int mqtt_port =8883;
```
+ Setting up wifi and MQTT broker details to connect with esp8266
```
void setup_wifi() {
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
```
+ Function to set up pin modes and current sensor
```
void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  dht.setup(DHTpin, DHTesp::DHT11); //Set up DHT11 sensor
  pinMode(bluePin, OUTPUT); //set up LED
  while (!Serial) delay(1);
  setup_wifi();
  #ifdef ESP8266
    espClient.setInsecure();
  #else
    espClient.setCACert(root_ca);      // enable this line and the the "certificate" code for secure connection
  #endif
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //current sensor
  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1);
  }
  uint32_t currentFrequency;
  Serial.println("Hello!");
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  Serial.println("Measuring voltage and current with INA219 ...");
}
```
+ Main function to implement logic and publish messages to MQTT broker and view in a client application
```
void loop() {
  if (!client.connected()) reconnect(); // check if client is connected
  client.loop();
long duration, distance;
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Send a pulse to trigger the sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Read the echo pin
  duration = pulseIn(echoPin, HIGH);
//read DHT11 temperature and humidity reading
  delay(dht.getMinimumSamplingPeriod());
  float humidity = dht.getHumidity();
  float power_mW = ina219.getPower_mW();
  ldrStatus=analogRead(ldrPin);
  //float brightness= 
  //float temperature = dht.getTemperature();
  // Calculate the distance in centimeters
  distance = duration * 0.034 / 2;
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  Serial.print("Hum: ");
  Serial.print(humidity);
  Serial.print("ldrStatus: ");
  Serial.print(ldrStatus);
  DynamicJsonDocument doc(1024);
  if (distance <= 20) {
  analogWrite(bluePin,255);
  }
  else if (distance > 20 && distance <=70) { // 60 <= temperature < 70
  analogWrite(bluePin,128);
  }
  else if(distance > 70 && distance <=160){
  analogWrite(bluePin,76);
  Serial.print("brightness: 30% ");
  }
  else{
    analogWrite(bluePin,0);
  }
  if(humidity>=70){
  analogWrite(redPin,255);
  }
  else if(humidity>50 && humidity <70){
    analogWrite(redPin,128);
  }
  else{
    analogWrite(redPin,0);
  }
if(ldrStatus>=20){
  analogWrite(greenPin,255);
}
else if(ldrStatus>10 && ldrStatus<20){
    analogWrite(greenPin,128);}
else
  analogWrite(greenPin,0);
  doc["deviceId"] = "NodeMCU";
  doc["siteId"] = "My Demo Lab";
  doc["humidity"] = humidity;
  //doc["temperature"] = temperature;
  doc["distance"] = distance;
  doc["power_mW"]=power_mW;
  doc["ldr_status"]=ldrStatus;
  char mqtt_message[128];
  serializeJson(doc, mqtt_message);
  publishMessage("smartstreet/sensors", mqtt_message, true);
  delay(10000);
}
```
## Results
Using the data got from the sensors,plots were generated using Python,for the 3 variables against power.
+ Power consumed against Distance of the object in the proximity of the Ultra sonic sensor
<img width="535" alt="power_dist" src="https://github.com/Donnybroke/smart_street_lighting/assets/92339420/ad409e1a-7386-41cc-bd8e-b5ad691ebce1">

+ Power consumed against Humidity recorded by Humidity sensor
<img width="556" alt="pow_hum" src="https://github.com/Donnybroke/smart_street_lighting/assets/92339420/7822668a-55b8-45c9-a408-0c9d1c52da3d">

+ Power consumed against Light resistance by LDR
<img width="613" alt="ldr_power" src="https://github.com/Donnybroke/smart_street_lighting/assets/92339420/11440689-6111-4ace-a6b4-a8f040af2d5d">

## Challenges and Improvements
1. Using an esp8266 isn't a stable device to implement pwm even though it's necessary for wifi connection.
1. Also some of the readings are not stable and undependable.
## Conclusion
Even with the challenges,the goal for the project was reached,and the fact that many technolgies come together to form a useful system is appreciated.
