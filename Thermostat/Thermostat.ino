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
#define heatCoolPowerPin 12
#define mainPowerPin 14
#define hotCoolPin 5 //LOW = Heat, HIGH = Cool
#define fanPin 13

const char* ssid = "";
const char* password = "";
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
uint oldHeatCoolState;
uint oldTargetHeatCoolState;
bool timerState;

String chipId_string;

IPAddress server(10, 1, 0, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);
DHT dht(DHTPIN, DHTTYPE);

float celsiusTemp, farenheitTemp, humidityTemp, targetTemperature, coolThresholdTemperature, heatThresholdTemperature, accessoryTempValue;
unsigned long previousMillis = 0;
unsigned long previousTempMillis = 0;
unsigned long currentTempMillis = 0;
unsigned long prevFanDelayMillis = 0;
unsigned long currentFanDelayMillis = 0;
unsigned long lastSetAccessoryState = 0;

const long interval = 5000;
unsigned long lasttemphumipublish = 0;
const long temphumipublishtime = 15000;
const long fanDelayTime = 15000;

void setup() {
    Serial.begin(74880);
    chipId_string = serviceType+String(ESP.getChipId());
    chipId_string.toCharArray(chipId,64);
    dht.begin();
    pinMode(mainPowerPin, OUTPUT);
    pinMode(heatCoolPowerPin, OUTPUT);
    pinMode(hotCoolPin, OUTPUT);
    pinMode(fanPin, OUTPUT);
    digitalWrite(mainPowerPin, LOW);
    digitalWrite(heatCoolPowerPin, HIGH); //Provides power to hot/cool
    digitalWrite(hotCoolPin, HIGH); //Low = Hot, High = Cool. Use NO NC on relay. Didn't want it to be possible for hot and cool to be on at the same time with possible freeze/crash.
    digitalWrite(fanPin, HIGH); // Gets power directly from 24v so fan can remain on after hot or cool is turned off. Later use with Air Quality Sensor
    currentHeatCoolState = 0;
    oldHeatCoolState = 0;
    targetHeatCoolState = 0;
    coolThresholdTemperature = 32;
    heatThresholdTemperature = 0;
    targetTemperature = 32;
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
    unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) { //Read Temp
    previousMillis = currentMillis;
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      celsiusTemp = 0;
      farenheitTemp = 0;
      humidityTemp = 0;
      }
      else{
        celsiusTemp = dht.computeHeatIndex(t, h, false);
        farenheitTemp = dht.computeHeatIndex(f, h);
        humidityTemp = h;
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.println();
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.print(" *C   ");
        Serial.print(f);
        Serial.print(" *F");
        Serial.println();
      }
  }
  currentFanDelayMillis = millis();
  if(((currentFanDelayMillis - lastSetAccessoryState) >= fanDelayTime) && (timerState == 1)){
    if(currentHeatCoolState == 1 || currentHeatCoolState == 2){
      digitalWrite(fanPin, LOW);
      }
    if(currentHeatCoolState == 0){
      digitalWrite(fanPin, HIGH);
    }
    timerState = 0;
    Serial.println("Fan Delay");
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (client.connect(MQTT::Connect(chipId)
       .set_auth(chipId, mqttpass))) {
        client.set_callback(callback);
        client.subscribe(intopic);
        client.subscribe(gettopic);
        client.subscribe(mainttopic);
        addAccessory();
        } 
        else {
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
    addThermostatJson["service"] = serviceType;
    addThermostatJson["TargetHeatingCoolingState"] = targetHeatCoolState;
    addThermostatJson["CurrentHeatingCoolingState"] = currentHeatCoolState;
    addThermostatJson["CurrentTemperature"] = celsiusTemp;
    addThermostatJson["TargetTemperature"] = targetTemperature;
    addThermostatJson["TemperatureDisplayUnits"] = 0;
    addThermostatJson["CurrentRelativeHumidity"] = humidityTemp;
    addThermostatJson["CoolingThresholdTemperature"] = coolThresholdTemperature;
    addThermostatJson["HeatingThresholdTemperature"] = heatThresholdTemperature;
    String addThermostatString;
    addThermostatJson.printTo(addThermostatString);
    client.publish(addtopic,addThermostatString);
    Serial.println("Added");
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
    if(accessoryCharacteristic == std::string("TargetTemperature")){
    setEnvJson["characteristic"] = "TargetTemperature";
      targetTemperature = accessoryTempValue;
      setEnvJson["value"] = accessoryTempValue;
    }
    else if(accessoryCharacteristic == std::string("TargetHeatingCoolingState")){
    setEnvJson["characteristic"] = "TargetHeatingCoolingState";
      targetHeatCoolState = accessoryValue;
      setEnvJson["value"] = accessoryValue;
      lastSetAccessoryState = millis();
      timerState = 1;
      setCurrentHeatCoolState();
    }
    else if(accessoryCharacteristic == std::string("TemperatureDisplayUnits")){
    setEnvJson["characteristic"] = "TemperatureDisplayUnits";
      tempDisplayUnits = accessoryValue;
      setEnvJson["value"] = accessoryValue;
    }
    else if(accessoryCharacteristic == std::string("CoolingThresholdTemperature")){
    setEnvJson["characteristic"] = "CoolingThresholdTemperature";
      coolThresholdTemperature = accessoryTempValue;
      setEnvJson["value"] = accessoryTempValue;
    }
    else if(accessoryCharacteristic == std::string("HeatingThresholdTemperature")){
    setEnvJson["characteristic"] = "HeatingThresholdTemperature";
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
    getEnvJson["value"] = celsiusTemp;
    }
    else if(accessoryCharacteristic == std::string("CurrentRelativeHumidity")){
    getEnvJson["value"] = humidityTemp;
    }
    else if(accessoryCharacteristic == std::string("CurrentHeatingCoolingState")){
      getEnvJson["value"] = currentHeatCoolState;
    }
    else if(accessoryCharacteristic == std::string("TargetTemperature")){
      getEnvJson["value"] = targetTemperature;
    }
    else if(accessoryCharacteristic == std::string("TargetHeatingCoolingState")){
      getEnvJson["value"] = targetHeatCoolState;
    }
    else if(accessoryCharacteristic == std::string("TemperatureDisplayUnits")){
      getEnvJson["value"] = tempDisplayUnits;
    }
    else if(accessoryCharacteristic == std::string("CoolingThresholdTemperature")){
      getEnvJson["value"] = coolThresholdTemperature;
    }
    else if(accessoryCharacteristic == std::string("HeatingThresholdTemperature")){
      getEnvJson["value"] = heatThresholdTemperature;
    }
    String getEnvString;
    getEnvJson.printTo(getEnvString);
    client.publish(outtopic,getEnvString);
}

void publishtemperature(){
    StaticJsonBuffer<200> getTempBuffer;
    JsonObject& getTempJson = getTempBuffer.createObject();
    getTempJson["name"] = chipId;
    getTempJson["characteristic"] = "CurrentTemperature";
    getTempJson["value"] = celsiusTemp;
    String getTempString;
    getTempJson.printTo(getTempString);
    client.publish(outtopic,getTempString);

    StaticJsonBuffer<200> getHumiBuffer;
    JsonObject& getHumiJson = getHumiBuffer.createObject();
    getHumiJson["name"] = chipId;
    getHumiJson["characteristic"] = "CurrentRelativeHumidity";
    getHumiJson["value"] = humidityTemp;
    String getHumiString;
    getHumiJson.printTo(getHumiString);
    client.publish(outtopic,getHumiString);
    Serial.println("Publish Temperature");
}

void publishHeatCoolState(){
    StaticJsonBuffer<200> pubHeatCoolBuffer;
    JsonObject& pubHeatCoolJson = pubHeatCoolBuffer.createObject();
    pubHeatCoolJson["name"] = chipId;
    pubHeatCoolJson["characteristic"] = "CurrentHeatingCoolingState";
    if(currentHeatCoolState = 0){
      pubHeatCoolJson["value"] = 0;
      oldHeatCoolState = 0;
      }
    else if(currentHeatCoolState = 1){
      pubHeatCoolJson["value"] = 1;
      oldHeatCoolState = 1;
    }
    else if(currentHeatCoolState = 2){
      pubHeatCoolJson["value"] = 2;
      oldHeatCoolState = 2;
    }
    String pubHeatCoolString;
    pubHeatCoolJson.printTo(pubHeatCoolString);
    client.publish(outtopic,pubHeatCoolString);
    Serial.print("Publish Heat Cool State ");
    Serial.println(currentHeatCoolState);
}

void setCurrentHeatCoolState(){
  if(targetHeatCoolState = 0){ //Off
    digitalWrite(heatCoolPowerPin, HIGH);
    digitalWrite(hotCoolPin, HIGH);
    digitalWrite(fanPin, HIGH);
    currentHeatCoolState = 0;
    Serial.println("Set Current Heat Cool State 0");
    }
  else if((targetHeatCoolState = 1) && (celsiusTemp < targetTemperature)){ //Heat On
    currentHeatCoolState = 1;
    digitalWrite(hotCoolPin, LOW);
    digitalWrite(heatCoolPowerPin, HIGH);
    }
  else if((targetHeatCoolState = 1) && (celsiusTemp >= targetTemperature)){ //Heat Off
    currentHeatCoolState = 1;
    digitalWrite(heatCoolPowerPin, LOW);
    digitalWrite(hotCoolPin, LOW);
    }
  else if((targetHeatCoolState = 2) && (celsiusTemp > targetTemperature)){ //Cool On
    currentHeatCoolState = 2;
    digitalWrite(hotCoolPin, HIGH);
    digitalWrite(heatCoolPowerPin, HIGH);
    }
  else if((targetHeatCoolState = 2) && (celsiusTemp <= targetTemperature)){ //Cool Off
    currentHeatCoolState = 2;
    digitalWrite(heatCoolPowerPin, LOW);
    digitalWrite(hotCoolPin, HIGH);
    }
  else if(targetHeatCoolState = 3){ //Auto
    if(celsiusTemp < heatThresholdTemperature){ //Heat On
      currentHeatCoolState = 1;
      digitalWrite(hotCoolPin, LOW);
      digitalWrite(heatCoolPowerPin, HIGH);
    }
    else if(celsiusTemp >= heatThresholdTemperature){ //Heat Off
      currentHeatCoolState = 0;
      digitalWrite(heatCoolPowerPin, LOW);
      digitalWrite(hotCoolPin, LOW);
    }
    else if(celsiusTemp > coolThresholdTemperature){ //Cool On
      currentHeatCoolState = 2;
      digitalWrite(hotCoolPin, HIGH);
      digitalWrite(heatCoolPowerPin, HIGH);
    }
    else if(celsiusTemp <= coolThresholdTemperature){ //Cool Off
      currentHeatCoolState = 0;
      digitalWrite(heatCoolPowerPin, LOW);
      digitalWrite(hotCoolPin, HIGH);
    }
  }
  publishHeatCoolState();
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
      else if(accessoryCharacteristic == std::string("TargetTemperature") || accessoryCharacteristic == std::string("CoolingThresholdTemperature") || accessoryCharacteristic == std::string("HeatingThresholdTemperature")){
        accessoryTempValue = mqttAccessory["value"];
      }
      setAccessory();
      Serial.println("Set ");
      Serial.print(accessoryCharacteristic);
      Serial.print(" to ");
    }
  }
    else if (gettopic == std::string(pubTopic)){
      if (accessoryName == std::string(chipId)){
        accessoryCharacteristic = mqttAccessory["characteristic"];
        getAccessory();
        Serial.println("Get ");
        Serial.print(accessoryCharacteristic);
      }
  }
}

