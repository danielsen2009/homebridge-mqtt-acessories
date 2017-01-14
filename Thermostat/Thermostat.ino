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
uint accessoryValue;
const char* accessoryValueString;
uint targetHeatCoolState;
uint currentHeatCoolState;
uint tempDisplayUnits;

String chipId_string;

IPAddress server(10, 1, 1, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);
DHT dht(DHTPIN, DHTTYPE, 11);

float humi_d, temp_df, targetTemperature, coolThresholdTemperature, heatThresholdTemperature, accessoryTempValue;
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
    currentHeatCoolState = 0;
    targetHeatCoolState = 0;
    coolThresholdTemperature = 75.0;
    heatThresholdTemperature = 68.0;
    targetTemperature = 70.0;
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
    setHeatCoolState();
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
    addThermostatJson["TargetHeatingCoolingState"] = targetHeatCoolState;
    addThermostatJson["CurrentHeatingCoolingState"] = currentHeatCoolState;
    addThermostatJson["CurrentTemperature"] = temp_df;
    addThermostatJson["TargetTemperature"] = targetTemperature;
    addThermostatJson["TemperatureDisplayUnits"] = 1;
    addThermostatJson["CurrentRelativeHumidity"] = humi_d;
    addThermostatJson["CoolingThresholdTemperature"] = coolThresholdTemperature;
    addThermostatJson["HeatingThresholdTemperature"] = heatThresholdTemperature;
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
    if(accessoryCharacteristic == std::string("TargetTemperature")){
      targetTemperature = accessoryTempValue;
      setEnvJson["value"] = accessoryTempValue;
    }
    if(accessoryCharacteristic == std::string("TargetHeatingCoolingState")){
      targetHeatCoolState = accessoryValue;
      setEnvJson["value"] = accessoryValue;
    }
    if(accessoryCharacteristic == std::string("TemperatureDisplayUnits")){
      tempDisplayUnits = accessoryValue;
      setEnvJson["value"] = accessoryValue;
    }
    if(accessoryCharacteristic == std::string("CoolingThresholdTemperature")){
      coolThresholdTemperature = accessoryTempValue;
      setEnvJson["value"] = accessoryTempValue;
    }
    if(accessoryCharacteristic == std::string("HeatingThresholdTemperature")){
      heatThresholdTemperature = accessoryTempValue;
      setEnvJson["value"] = accessoryTempValue;
    }
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
    if(accessoryCharacteristic == std::string("CurrentHeatingCoolingState")){
      getEnvJson["value"] = currentHeatCoolState;
    }
    if(accessoryCharacteristic == std::string("TargetTemperature")){
      getEnvJson["value"] = targetTemperature;
    }
    if(accessoryCharacteristic == std::string("TargetHeatingCoolingState")){
      getEnvJson["value"] = targetHeatCoolState;
    }
    if(accessoryCharacteristic == std::string("TemperatureDisplayUnits")){
      getEnvJson["value"] = tempDisplayUnits;
    }
    if(accessoryCharacteristic == std::string("CoolingThresholdTemperature")){
      getEnvJson["value"] = coolThresholdTemperature;
    }
    if(accessoryCharacteristic == std::string("HeatingThresholdTemperature")){
      getEnvJson["value"] = heatThresholdTemperature;
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

void setHeatCoolState(){
  if(targetHeatCoolState = 0){ //Off
    digitalWrite(powerPin, LOW); //Provides power to hot/cool
    digitalWrite(hotCoolPin, LOW); //Low = Hot, High = Cool. Use NO NC on relay. Didn't want it to be possible for hot and cool to be on at the same time with possible freeze/crash.
    digitalWrite(fanPin, LOW);
    currentHeatCoolState = 0;
  }
  if(targetHeatCoolState = 1){ //Heat
    currentHeatCoolState = 1;
    if(temp_df < targetTemperature){
      digitalWrite(powerPin, HIGH); //Provides power to hot/cool
      digitalWrite(hotCoolPin, LOW); //Low = Hot, High = Cool. Use NO NC on relay. Didn't want it to be possible for hot and cool to be on at the same time with possible freeze/crash.
      digitalWrite(fanPin, HIGH);
    }
    if(temp_df >= targetTemperature){
      digitalWrite(powerPin, LOW); //Provides power to hot/cool
      digitalWrite(hotCoolPin, LOW); //Low = Hot, High = Cool. Use NO NC on relay. Didn't want it to be possible for hot and cool to be on at the same time with possible freeze/crash.
      digitalWrite(fanPin, LOW);
    }
  }
  if(targetHeatCoolState = 2){ //Cool
    currentHeatCoolState = 2;
    if(temp_df > targetTemperature){
      digitalWrite(powerPin, HIGH); //Provides power to hot/cool
      digitalWrite(hotCoolPin, HIGH); //Low = Hot, High = Cool. Use NO NC on relay. Didn't want it to be possible for hot and cool to be on at the same time with possible freeze/crash.
      digitalWrite(fanPin, HIGH);
    }
    if(temp_df <= targetTemperature){
      digitalWrite(powerPin, LOW); //Provides power to hot/cool
      digitalWrite(hotCoolPin, HIGH); //Low = Hot, High = Cool. Use NO NC on relay. Didn't want it to be possible for hot and cool to be on at the same time with possible freeze/crash.
      digitalWrite(fanPin, LOW);
    }
  }
  if(targetHeatCoolState = 3){ //Auto
    if(temp_df < heatThresholdTemperature){
      currentHeatCoolState = 1;
      digitalWrite(powerPin, HIGH);
      digitalWrite(hotCoolPin, LOW);
      digitalWrite(fanPin, HIGH);
    }
    if(temp_df >= heatThresholdTemperature){
      currentHeatCoolState = 0;
      digitalWrite(powerPin, LOW);
      digitalWrite(hotCoolPin, LOW);
      digitalWrite(fanPin, LOW);
    }
    if(temp_df > coolThresholdTemperature){
      currentHeatCoolState = 2;
      digitalWrite(powerPin, HIGH);
      digitalWrite(hotCoolPin, HIGH);
      digitalWrite(fanPin, HIGH);
    }
    if(temp_df <= coolThresholdTemperature){
      currentHeatCoolState = 0;
      digitalWrite(powerPin, LOW);
      digitalWrite(hotCoolPin, HIGH);
      digitalWrite(fanPin, LOW);
    }
  }
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
      if(accessoryCharacteristic == std::string("TargetHeatingCoolingState") || accessoryCharacteristic == std::string("TemperatureDisplayUnits")){
      accessoryValue = mqttAccessory["value"];
      }
      if(accessoryCharacteristic == std::string("TargetTemperature") || accessoryCharacteristic == std::string("CoolingThresholdTemperature") || accessoryCharacteristic == std::string("HeatingThresholdTemperature")){
        accessoryTempValue = mqttAccessory["value"];
      }
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
