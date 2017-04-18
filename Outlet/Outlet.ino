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
#define topOutlet 16
#define bottomOutlet 14

const char* ssid = "HomeBridge202";
const char* password = "HomeBridgePassPhrase";
uint chipId_int;
uint16_t i;
char serviceType[256] = "Outlet";
char chipId[256];
char chipIdA[256];
char chipIdB[256];
char pubTopic[256];
char pubMessage[256];

long currentMillis = 0;
long previousMillis = 0;
long interval = 120000;

const char* mqttpass = "";
const char* intopic = "homebridge/from/set";
const char* outtopic = "homebridge/to/set";
const char* gettopic = "homebridge/from/get";
const char* addtopic = "homebridge/to/add";
const char* removetopic = "homebridge/to/remove";
const char* mainttopic = "homebridge/from/connected";
const char* servicetopic = "homebridge/to/add/service";
const char* reachabilitytopic = "homebridge/to/set/reachability";
const char* accessoryInformationTopic = "homebridge/to/set/accessoryinformation";
const char* maintmessage = "";
const char* accessoryName;
const char* accessoryCharacteristic;
const char* accessoryServiceName;
const char* accessoryValue;
const char* accessoryValueString;
bool currentOutletState;
bool topOutletState;
bool bottomOutletState;

String chipId_string;
String chipId_stringA;
String chipId_stringB;
String jsonReachabilityString;

IPAddress server(10, 1, 0, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
  Serial.begin(74880);
  chipId_string = String(serviceType)+String(ESP.getChipId());
  chipId_stringA = String(serviceType)+"A"+String(ESP.getChipId());
  chipId_stringB = String(serviceType)+"B"+String(ESP.getChipId());
  chipId_string.toCharArray(chipId,256);
  chipId_stringA.toCharArray(chipIdA,256);
  chipId_stringB.toCharArray(chipIdB,256);
  pinMode(topOutlet, OUTPUT);
  pinMode(bottomOutlet, OUTPUT);
  digitalWrite(topOutlet, HIGH);
  digitalWrite(bottomOutlet, HIGH);
  topOutletState = false;
  bottomOutletState = false;
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
    currentMillis = millis();
    if(currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;
      setReachability();
      }
    client.loop();
    }else{
      wifi_conn();
  }
}

void addAccessory(){
  StaticJsonBuffer<200> jsonAccessoryInformationBuffer;
  JsonObject& accessoryInformation = jsonAccessoryInformationBuffer.createObject();
  accessoryInformation["name"] = chipId;
  accessoryInformation["manufacturer"] ="StaticIp IT";
  accessoryInformation["model"] ="Dual WiFi Controlled Outlet (1.0)";
  accessoryInformation["serialnumber"] = ESP.getChipId();
  String jsonAccessoryInformationString;
  accessoryInformation.printTo(jsonAccessoryInformationString);
  Serial.println(jsonAccessoryInformationString);
  client.publish(accessoryInformationTopic, jsonAccessoryInformationString);
  Serial.println();
  
  StaticJsonBuffer<200> jsonOutletBufferA;
  JsonObject& addOutletAccessoryJsonA = jsonOutletBufferA.createObject();
  addOutletAccessoryJsonA["name"] = chipId;
  addOutletAccessoryJsonA["service"] = serviceType;
  addOutletAccessoryJsonA["reachable"] = true;
  String addOutletAccessoryJsonAString;
  addOutletAccessoryJsonA.printTo(addOutletAccessoryJsonAString);
  Serial.println(addOutletAccessoryJsonAString);
  client.publish(addtopic,addOutletAccessoryJsonAString);
  Serial.println("Outlet 'A' Added");

  //Add Service
  StaticJsonBuffer<200> jsonOutletBufferB;
  JsonObject& addOutletAccessoryJsonB = jsonOutletBufferB.createObject();
  addOutletAccessoryJsonB["name"] = chipId;
  addOutletAccessoryJsonB["service_name"] = chipIdB;
  addOutletAccessoryJsonB["service"] = serviceType;
  String addOutletAccessoryJsonBString;
  addOutletAccessoryJsonB.printTo(addOutletAccessoryJsonBString);
  Serial.println(addOutletAccessoryJsonBString);
  client.publish(servicetopic,addOutletAccessoryJsonBString);
  Serial.println("Outlet 'B' Added");
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
    Serial.println();
    Serial.print("Set Outlet ");
    if(currentOutletState == true){
      if(accessoryServiceName == std::string(chipId))
      {
        Serial.println("A ON");
        topOutletState = true;
        digitalWrite(topOutlet, LOW);
      }
      else if(accessoryServiceName == std::string(chipIdB))
      {
        Serial.println("B ON");
        bottomOutletState = true;
        digitalWrite(bottomOutlet, LOW);
      }
    }
    else if(currentOutletState == false)
    {
      if(accessoryServiceName == std::string(chipId))
      {
        Serial.println("A OFF");
        topOutletState = false;
        digitalWrite(topOutlet, HIGH);
      }
      else if(accessoryServiceName == std::string(chipIdB))
      {
        Serial.println("B OFF");
        bottomOutletState = false;
        digitalWrite(bottomOutlet, HIGH);
    }
  }
}

void getAccessory(){
  Serial.print("Get Outlet");
  StaticJsonBuffer<200> getOutletBuffer;
  JsonObject& getOutletJson = getOutletBuffer.createObject();
  if(accessoryServiceName == std::string(chipId))
  {
    getOutletJson["name"] = chipId;
    getOutletJson["characteristic"] = "On";
    getOutletJson["value"] = topOutletState;
    Serial.print(" A: ");
    Serial.println(topOutletState);
  }
  else if(accessoryServiceName == std::string(chipIdB))
  {
    getOutletJson["name"] = chipId;
    getOutletJson["service_name"] = chipIdB;
    getOutletJson["characteristic"] = "On";
    getOutletJson["value"] = bottomOutletState;
    Serial.print(" B: ");
    Serial.println(bottomOutletState);
  }
  String getOutletString;
  getOutletJson.printTo(getOutletString);
  client.publish(outtopic, getOutletString);
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
    if(accessoryName == std::string(chipId) && (accessoryServiceName == std::string(chipId) || accessoryServiceName == std::string(chipIdB))){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      currentOutletState = mqttAccessory["value"];
      setAccessory();
    }
  }
  else if(gettopic == std::string(pubTopic)){
    if(accessoryName == std::string(chipId) && (accessoryServiceName == std::string(chipId) || accessoryServiceName == std::string(chipIdB))){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      getAccessory();
    }
  }
  else if(accessoryName != std::string(chipId)){
    //
  }
}
