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
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop(){
    ArduinoOTA.handle();
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
        gettemperature();
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
    StaticJsonBuffer<200> setSwitchBuffer;
    JsonObject& setSwitchJson = setSwitchBuffer.createObject();
    setSwitchJson["name"] = chipId;
    setSwitchJson["characteristic"] = accessoryCharacteristic;
    setSwitchJson["value"] = "";
    String setSwitchString;
    setSwitchJson.printTo(setSwitchString);
    client.publish(outtopic,setSwitchString);
  }

void getAccessory(){
    StaticJsonBuffer<200> getSwitchBuffer;
    JsonObject& getSwitchJson = getSwitchBuffer.createObject();
    getSwitchJson["name"] = accessoryName;
    getSwitchJson["characteristic"] = accessoryCharacteristic;
    getSwitchJson["value"] = temp_df;
    getSwitchJson["value"] = humi_d;
    String getSwitchString;
    getSwitchJson.printTo(getSwitchString);
    client.publish(outtopic,getSwitchString);
}

void gettemperature() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    humi_d = dht.readHumidity();
    temp_df = dht.readTemperature(true);
    
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
