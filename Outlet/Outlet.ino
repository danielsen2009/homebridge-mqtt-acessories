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

const char* ssid = "SSID";
const char* password = "PASSWORD";
uint chipId_int;
uint16_t i;
char serviceType[32] = "Outlet";
char chipId[64];
char chipIdA[32];
char chipIdB[32];
char pubTopic[32];
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
bool currentOutletState;
bool topOutletState;
bool bottomOutletState;

String chipId_string;
String chipId_stringA;
String chipId_stringB;

IPAddress server(10, 1, 1, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
  Serial.begin(74880);
  chipId_string = String(serviceType)+String(ESP.getChipId());
  chipId_stringA = String(serviceType)+"A"+String(ESP.getChipId());
  chipId_stringB = String(serviceType)+"B"+String(ESP.getChipId());
  chipId_string.toCharArray(chipId,64);
  chipId_stringA.toCharArray(chipIdA,32);
  chipId_stringB.toCharArray(chipIdB,32);
  #define topOutlet 16
  #define bottomOutlet 14
  pinMode(topOutlet, OUTPUT);
  pinMode(bottomOutlet, OUTPUT);
  digitalWrite(topOutlet, HIGH);
  digitalWrite(bottomOutlet, HIGH);
  topOutletState = false;
  bottomOutletState = false;
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
       .set_auth(chipId, mqttpass))) {
  client.set_callback(callback);
  client.subscribe(intopic);
  client.subscribe(gettopic);
  client.subscribe(mainttopic);
  addAccessory();
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
  StaticJsonBuffer<200> jsonOutletBufferA;
  StaticJsonBuffer<200> jsonOutletBufferB;
  JsonObject& addOutletAccessoryJsonA = jsonOutletBufferA.createObject();
  JsonObject& addOutletAccessoryJsonB = jsonOutletBufferB.createObject();
  addOutletAccessoryJsonA["name"] = chipIdA;
  addOutletAccessoryJsonB["name"] = chipIdB;
  addOutletAccessoryJsonA["service"] = serviceType;
  addOutletAccessoryJsonB["service"] = serviceType;
  addOutletAccessoryJsonA["OutletInUse"] = true;
  addOutletAccessoryJsonB["OutletInUse"] = true;
  String addOutletAccessoryJsonAString;
  String addOutletAccessoryJsonBString;
  addOutletAccessoryJsonA.printTo(addOutletAccessoryJsonAString);
  addOutletAccessoryJsonB.printTo(addOutletAccessoryJsonBString);
  Serial.println(addOutletAccessoryJsonAString);
  Serial.println(addOutletAccessoryJsonBString);
  client.publish(addtopic,addOutletAccessoryJsonAString);
  client.publish(addtopic,addOutletAccessoryJsonBString);
  Serial.println("Outlet Added");
}

void maintAccessory(){
  Serial.println("Maintenance");
}

void removeAccessory(){
  Serial.println("Remove");
}

void setAccessory(){
    Serial.println();
    Serial.println(currentOutletState);
    Serial.print("Set Outlet ");
    if(currentOutletState == true){
      if(accessoryName == std::string(chipIdA))
      {
        Serial.println("A ON");
        topOutletState = true;
        digitalWrite(topOutlet, LOW);
      }
      else if(accessoryName == std::string(chipIdB))
      {
        Serial.println("B ON");
        bottomOutletState = true;
        digitalWrite(bottomOutlet, LOW);
      }
    }
    else if(currentOutletState == false)
    {
      if(accessoryName == std::string(chipIdA))
      {
        Serial.println("A OFF");
        topOutletState = false;
        digitalWrite(topOutlet, HIGH);
      }
      else if(accessoryName == std::string(chipIdB))
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
  if(accessoryName == std::string(chipIdA))
  {
    getOutletJson["name"] = chipIdA;
    getOutletJson["characteristic"] = "On";
    getOutletJson["value"] = topOutletState;
  }
  else if(accessoryName == std::string(chipIdB))
  {
    getOutletJson["name"] = chipIdB;
    getOutletJson["characteristic"] = "On";
    getOutletJson["value"] = bottomOutletState;
  }
  String getOutletString;
  getOutletJson.printTo(getOutletString);
  client.publish(outtopic, getOutletString);
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
    if(serviceType == std::string("Outlet") && accessoryName == std::string(chipIdA)){
      Serial.print("Top Outlet");
      accessoryCharacteristic = mqttAccessory["characteristic"];
      currentOutletState = mqttAccessory["value"];
      setAccessory();
    }
    else if(serviceType == std::string("Outlet") && accessoryName == std::string(chipIdB)){
      Serial.print("Bottom Outlet");
      accessoryCharacteristic = mqttAccessory["characteristic"];
      currentOutletState = mqttAccessory["value"];
      setAccessory();
    }
  }
  else if(accessoryName != std::string(chipId)){
    //
  }
}
