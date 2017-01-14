// Includes //
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <DHT.h>
#define DHTTYPE DHT11
#define DHTPIN  16
#define powerPin 0
#define hotCoolPin 4
#define fanPin 5

const char* ssid = "SSID";
const char* password = "Password";
uint chipId_int;
uint16_t i;
char serviceType[64] = "Thermostat";
char chipId[64];
char pubTopic[64];
char pubMessage[256];

const char* mqttpass = "";
const char* intopic = "homebridge/from/set";
const char* outtopic = "homebridge/to/set";
const char* gettopic = "homebridge/from/get";
const char* addtopic = "homebridge/to/add";
const char* removetopic = "homebridge/to/remove";
const char* mainttopic = "homebridge/from/connected";
const char* maintmessage = "";
const char* accessoryName;
const char* accessoryCharacteristic;
const char* accessoryValue;
const char* accessoryValueString;

String chipId_string;

IPAddress server(10, 1, 1, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);
DHT dht(DHTPIN, DHTTYPE, 11);

float humi_d, temp_df;
unsigned long previousMillis = 0;
unsigned long previousTempMillis = 0;
unsigned long currentTempMillis = 0;

const long interval = 2000;
unsigned long lasttemphumipublish = 0;
const long temphumipublishtime = 60000;

void setup() {
    Serial.begin(74880);
    chipId_string = serviceType+String(ESP.getChipId());
    chipId_string.toCharArray(chipId,64);
    dht.begin();
    pinMode(powerPin, OUTPUT);
    pinMode(hotCoolPin, OUTPUT);
    pinMode(fanPin, OUTPUT);
    digitalWrite(powerPin, LOW); //Provides power to hot/cool
    digitalWrite(hotCoolPin, LOW); //Low = Hot, High = Cool. Use NO NC on relay. Didn't want it to be possible for hot and cool to be on at the same time with possible freeze/crash.
    digitalWrite(fanPin, LOW); // Gets power directly from 24v so fan can remain on after hot or cool is turned off. Later use with Air Quality Sensor
    wifi_conn();
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(chipId);
    // ArduinoOTA.setPassword((const char *)"123");
    ArduinoOTA.onStart([]() {
    Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
    ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}

void wifi_conn(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void loop(){
    ArduinoOTA.handle();
    gettemperature();
    if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (client.connect(MQTT::Connect(chipId)
       .set_auth(chipId, mqttpass))) {
  client.set_callback(callback);
  client.subscribe(intopic);
  client.subscribe(gettopic);
  client.subscribe(mainttopic);
  addAccessory();
      } else {
        delay(5000);
      }
    }
    if (client.connected())
    currentTempMillis = millis();
      if(currentTempMillis - previousTempMillis >= temphumipublishtime){
        previousTempMillis = currentTempMillis;
        publishtemperature();
      }
      client.loop();
  } else {
    wifi_conn();
  }
}

void addAccessory(){
    StaticJsonBuffer<200> jsonAddThermostatBuffer;
    JsonObject& addThermostatJson = jsonAddThermostatBuffer.createObject();
    addThermostatJson["name"] = chipId;
    addThermostatJson["CurrentHeatingCoolingState"] = 0;
    addThermostatJson["TargetHeatingCoolingState"] = 0;
    addThermostatJson["CurrentTemperature"] = 0;
    addThermostatJson["TargetTemperature"] = 0;
    addThermostatJson["TemperatureDisplayUnits"] = 1;
    addThermostatJson["CurrentRelativeHumidity"] = 0;
    addThermostatJson["CoolingThresholdTemperature"] = 0;
    addThermostatJson["HeatingThresholdTemperature"] = 0;
    String addThermostatString;
    addThermostatJson.printTo(addThermostatString);
    client.publish(addtopic,addThermostatString);
}

void maintAccessory(){
  //
}

void removeAccessory(){
  //
}

void setAccessory(){
    StaticJsonBuffer<200> setEnvBuffer;
    JsonObject& setEnvJson = setEnvBuffer.createObject();
    setEnvJson["name"] = chipId;
    setEnvJson["characteristic"] = accessoryCharacteristic;
    setEnvJson["value"] = "";
    String setEnvString;
    setEnvJson.printTo(setEnvString);
    client.publish(outtopic,setEnvString);
  }

void getAccessory(){
    StaticJsonBuffer<200> getEnvBuffer;
    JsonObject& getEnvJson = getEnvBuffer.createObject();
    getEnvJson["name"] = accessoryName;
    getEnvJson["characteristic"] = accessoryCharacteristic;
    if(accessoryCharacteristic == std::string("CurrentTemperature")){
    getEnvJson["value"] = temp_df;
    }
    if(accessoryCharacteristic == std::string("CurrentRelativeHumidity")){
    getEnvJson["value"] = humi_d;
    }
    String getEnvString;
    getEnvJson.printTo(getEnvString);
    client.publish(outtopic,getEnvString);
}

void gettemperature() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    humi_d = dht.readHumidity();
    temp_df = dht.readTemperature(true);
  }
}

void publishtemperature(){
    StaticJsonBuffer<200> getTempBuffer;
    JsonObject& getTempJson = getTempBuffer.createObject();
    getTempJson["name"] = accessoryName;
    getTempJson["characteristic"] = "CurrentTemperature";
    getTempJson["value"] = temp_df;
    String getTempString;
    getTempJson.printTo(getTempString);
    client.publish(outtopic,getTempString);

    StaticJsonBuffer<200> getHumiBuffer;
    JsonObject& getHumiJson = getHumiBuffer.createObject();
    getHumiJson["name"] = accessoryName;
    getHumiJson["characteristic"] = "CurrentRelativeHumidity";
    getHumiJson["value"] = humi_d;
    String getHumiString;
    getHumiJson.printTo(getHumiString);
    client.publish(outtopic,getHumiString);
}

void callback(const MQTT::Publish& pub){
  pub.topic().toCharArray(pubTopic,50);
  StaticJsonBuffer<200> jsoncallbackBuffer;
  JsonObject& mqttAccessory = jsoncallbackBuffer.parseObject(pub.payload_string());
  accessoryName = mqttAccessory["name"];
  if(mainttopic == std::string(pubTopic)){
    Serial.print("Maintenance");
    maintAccessory();
  }
  else if(intopic == std::string(pubTopic)){
    if (accessoryName == std::string(chipId)){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      accessoryValue = mqttAccessory["value"];
      setAccessory();
    }
  }
    else if (gettopic == std::string(pubTopic)){
      if (accessoryName == std::string(chipId)){
        accessoryCharacteristic = mqttAccessory["characteristic"];
        getAccessory();
      }
  }
}
