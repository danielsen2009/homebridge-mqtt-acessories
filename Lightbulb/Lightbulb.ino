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
#define ledPin 15

const char* ssid = "HomeBridge202";
const char* password = "HomeBridgePassPhrase";
uint chipId_int;
uint16_t i;
char serviceType[256] = "Lightbulb";
char chipId[256];
char pubTopic[256];
char pubMessage[256];

const char* mqttpass = "";
const char* intopic = "homebridge/from/set";
const char* outtopic = "homebridge/to/set";
const char* gettopic = "homebridge/from/get";
const char* addtopic = "homebridge/to/add";
const char* removetopic = "homebridge/to/remove";
const char* mainttopic = "homebridge/from/connected";
const char* servicetopic = "homebridge/to/add/service";
const char* reachabilitytopic = "homebridge/to/set/reachability";
const char* maintmessage = "";
const char* accessoryName;
const char* accessoryCharacteristic;
const char* accessoryServiceName;
const char* accessoryValue;
const char* accessoryValueString;
bool lightBulbOn;
const char* lightBrightness;
const char* lightHue;
const char* lightSaturation;

String chipId_string;
String jsonReachabilityString;

IPAddress server(10, 1, 0, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
  Serial.begin(74880);
  chipId_string = String(serviceType)+String(ESP.getChipId());
  chipId_string.toCharArray(chipId,256);
  pinMode(ledPin, OUTPUT);
  lightBulbOn = false;
  lightBrightness = 0;
  lightHue = 0;
  lightSaturation = 0;
  StaticJsonBuffer<200> jsonReachabilityBuffer;
  JsonObject& jsonReachability = jsonReachabilityBuffer.createObject();
  jsonReachability["name"] = chipId;
  jsonReachability["reachable"] = false;
  jsonReachability.printTo(jsonReachabilityString);
  wifi_conn();
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(chipId);
  // No authentication by default
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
       .set_will(reachabilitytopic, jsonReachabilityString)
       .set_auth(chipId, mqttpass))) {
  client.set_callback(callback);
  client.subscribe(intopic);
  client.subscribe(gettopic);
  client.subscribe(mainttopic);
  addAccessory();
  setReachability();
      } else {
        Serial.println("Could not connect to MQTT server");
        delay(5000);
      }
    }

    if (client.connected())
    client.loop();
    }else{
      wifi_conn();
  }
}

void addAccessory(){
  StaticJsonBuffer<200> jsonLightbulbBuffer;
  JsonObject& addLightbulbAccessoryJson = jsonLightbulbBuffer.createObject();
  addLightbulbAccessoryJson["name"] = chipId;
  addLightbulbAccessoryJson["service"] = serviceType;
  addLightbulbAccessoryJson["Brightness"] = lightBrightness;
  addLightbulbAccessoryJson["Hue"] = lightHue;
  addLightbulbAccessoryJson["Saturation"] = lightSaturation;
  addLightbulbAccessoryJson["reachable"] = true;
  String addLightbulbAccessoryJsonString;
  addLightbulbAccessoryJson.printTo(addLightbulbAccessoryJsonString);
  Serial.println(addLightbulbAccessoryJsonString);
  client.publish(addtopic,addLightbulbAccessoryJsonString);
  Serial.println("Lightbulb Added");
}

void maintAccessory(){
  Serial.println("Maintenance");
}

void setReachability(){
  StaticJsonBuffer<200> jsonSetReachability;
  JsonObject& setReachabilityJson = jsonSetReachability.createObject();
  setReachabilityJson["name"] = chipId;
  setReachabilityJson["reachable"] = true;
  String setReachabilityJsonString;
  setReachabilityJson.printTo(setReachabilityJsonString);
  Serial.println(setReachabilityJsonString);
  client.publish(reachabilitytopic,setReachabilityJsonString);
}

void removeAccessory(){
  Serial.println("Remove");
}

void setAccessory(){
    Serial.print("Set Lightbulb ");
    Serial.print(accessoryCharacteristic);
    Serial.print(" to ");
    if(accessoryCharacteristic == std::string("On")){
    lightBulbOn = accessoryValue;
  }
  else if(accessoryCharacteristic == std::string("Brightness")){
    lightBrightness = accessoryValue;
  }
  else if(accessoryCharacteristic == std::string("Hue")){
    lightHue = accessoryValue;
  }
  else if(accessoryCharacteristic == std::string("Saturation")){
    lightSaturation = accessoryValue;
  }
  Serial.println(accessoryValue);
}

void getAccessory(){
  Serial.print("Get Lightbulb ");
  Serial.print(accessoryCharacteristic);
  Serial.print(": ");
  if(accessoryCharacteristic == std::string("On")){
    Serial.println(lightBulbOn);
  }
  else if(accessoryCharacteristic == std::string("Brightness")){
    Serial.println(lightBrightness);
  }
  else if(accessoryCharacteristic == std::string("Hue")){
    Serial.println(lightHue);
  }
  else if(accessoryCharacteristic == std::string("Saturation")){
    Serial.println(lightSaturation);
  }
}

void callback(const MQTT::Publish& pub){
  pub.topic().toCharArray(pubTopic,512);
  StaticJsonBuffer<512> jsoncallbackBuffer;
  JsonObject& mqttAccessory = jsoncallbackBuffer.parseObject(pub.payload_string());
  accessoryName = mqttAccessory["name"];
  accessoryServiceName = mqttAccessory["service_name"];
  if(mainttopic == std::string(pubTopic)){
    Serial.print("Maintenance");
    maintAccessory();
  }
  else if(intopic == std::string(pubTopic)){
    if(accessoryName == std::string(chipId)){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      accessoryValue = mqttAccessory["value"];
      setAccessory();
    }
  }
  else if(gettopic == std::string(pubTopic)){
    if(accessoryName == std::string(chipId)){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      getAccessory();
    }
  }
  else if(accessoryName != std::string(chipId)){
    //
  }
}
