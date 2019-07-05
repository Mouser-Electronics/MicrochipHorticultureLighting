/******************************************************************
 * Project:
 * Version:
 * Date:
 ******************************************************************/

#include <ArduinoMqttClient.h>
#include <WiFi101.h>
#include "secrets.h"

char ssid[] = SECRET_SSID;        
char pass[] = SECRET_PASS;   

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = SECRET_BROKER;
const int  port     = SECRET_PORT;
const char topic[]  = SECRET_TOPIC;
const char subtopic[] = SECRET_SUBTOPIC;

const char pubMessageStart[] = "{\"event_data\":{\"lux\":\"";
const char pubMessageEnd[] = "\"}}";


const int lightSensorPin = 3;
const int DIMControlPin = 7;
const int ENControlPin = 5;

const long interval = 1000;
unsigned long previousMillis = 0;
unsigned int manualBrightSetPoint = 100;
unsigned int setBrightPWM = 255;
unsigned int sensorBrightnessReading = 0;
bool manualMode_ON = false;



/******************************************************************
 * Function:
 * Input:
 * Return:
 ******************************************************************/
void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();

  mqttClient.setUsernamePassword(SECRET_USERNAME, SECRET_PASSWORD);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  mqttClient.subscribe(subtopic);

  digitalWrite(ENControlPin, HIGH);
  analogWrite(DIMControlPin, 255); 
}



/******************************************************************
 * Function:
 * Input:
 * Return:
 ******************************************************************/
void loop() {
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();


  sensorBrightnessReading = analogRead(lightSensorPin);
  switch (manualMode_ON) {
    case true:
      digitalWrite(ENControlPin, HIGH);
      setBrightPWM = map(manualBrightSetPoint, 0, 100, 0, 255);
      analogWrite(DIMControlPin, setBrightPWM);
      break;
      
    default:
      digitalWrite(ENControlPin, HIGH);
      setBrightPWM = map(sensorBrightnessReading, 0, 1023, 255, 0);
      analogWrite(DIMControlPin, setBrightPWM);
      break;
  }
    
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    // save the last time a message was sent
    previousMillis = currentMillis;

    Serial.print(F("Sending message to topic: "));
    Serial.println(topic);
    Serial.print(pubMessageStart);
    Serial.print(String(sensorBrightnessReading, DEC));
    Serial.print(pubMessageEnd);
    Serial.println();

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(pubMessageStart);
    mqttClient.print(String(sensorBrightnessReading, DEC));
    mqttClient.print(pubMessageEnd);
    mqttClient.endMessage();
  }
}


/******************************************************************
 * Function:
 * Input:
 * Return:
 ******************************************************************/
void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.println(F("Received a message with topic '"));
  Serial.print(mqttClient.messageTopic());
  Serial.print(F("', length "));
  Serial.print(messageSize);
  Serial.println(F(" bytes:"));

  char messagePayload [messageSize];
  int i = 0;

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    messagePayload[i] = (char)mqttClient.read();
    Serial.print(messagePayload[i]);
    i++;
  }
  Serial.println();

  String payload = String(messagePayload);
  payload.trim();

  if(payload == String(F("manualControl_ON"))) {
    manualMode_ON = true;
  }
  else if (payload == String(F("manualControl_OFF"))) {
    manualMode_ON = false;
  }
  else {
    manualBrightSetPoint = payload.toInt();
  }
}
