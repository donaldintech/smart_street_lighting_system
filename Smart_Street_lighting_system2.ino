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
const int  bluePin=14,redPin=15,DHTpin=12,ldrPin=A0,greenPin=13;
DHTesp dht;
int ldrStatus;
Adafruit_INA219 ina219;

//code to run ultrasonic sensor
const int trigPin = 2;  // Trigger pin of the ultrasonic sensor
const int echoPin = 0; // Echo pin of the ultrasonic sensor

/****** WiFi Connection Details *******/
const char* ssid = "xxxxxx";
const char* password = "xxxxxx";

/******* MQTT Broker Connection Details *******/
const char* mqtt_server = "xxxxxxxx";
const char* mqtt_username = "xxxxxx";
const char* mqtt_password = "xxxxxxx";
const int mqtt_port =8883;
/**** Secure WiFi Connectivity Initialisation *****/
WiFiClientSecure espClient;

/**** MQTT Client Initialisation Using WiFi Connection *****/
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
/****** root certificate *********/

static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
xxxxxxxxx
xxxxxxxxx
-----END CERTIFICATE-----
)EOF";
/************* Connect to WiFi ***********/
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
/************* Connect to MQTT Broker ***********/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");

      client.subscribe("led_state");   // subscribe the topics here

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
/***** Call back Method for Receiving MQTT messages and Switching LED ****/

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];

  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);

}
/**** Method for Publishing MQTT Messages **********/
void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message published ["+String(topic)+"]: "+payload);
}
/**** Application Initialisation Function******/
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
/******** Main Function *************/
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
