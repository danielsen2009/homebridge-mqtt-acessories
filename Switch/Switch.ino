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
const char* password = "Password";
uint chipId_int;
uint16_t i;
char serviceType[64] = "Switch";
char chipId[64];
char chipIdA[64];
char chipIdB[64];
char chipIdC[64];
char chipIdD[64];
char chipIdE[64];
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
bool currentSwitchAState;
bool currentSwitchBState;
bool currentSwitchCState;
bool currentSwitchDState;
bool oldSwitchAState;
bool oldSwitchBState;
bool oldSwitchCState;
bool oldSwitchDState;
bool digitalSwitchAState;
bool digitalSwitchBState;
bool digitalSwitchCState;
bool digitalSwitchDState;
bool switchState;
bool switchAState;
bool switchBState;
bool switchCState;
bool switchDState;

String chipId_string;
String chipId_stringA;
String chipId_stringB;
String chipId_stringC;
String chipId_stringD;
String chipId_stringE;
String chipId_stringF;
String chipId_stringG;

unsigned long lastDebounceATime = 0;
unsigned long lastDebounceBTime = 0;
unsigned long lastDebounceCTime = 0;
unsigned long lastDebounceDTime = 0;
unsigned long debounceDelay = 60;

IPAddress server(10, 1, 1, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
    Serial.begin(74880);
    chipId_string = serviceType+String(ESP.getChipId());
    chipId_stringA = "SwitchA"+String(ESP.getChipId());
    chipId_stringB = "SwitchB"+String(ESP.getChipId());
    chipId_stringC = "SwitchC"+String(ESP.getChipId());
    chipId_stringC = "SwitchD"+String(ESP.getChipId());
    chipId_string.toCharArray(chipId,64);
    chipId_stringA.toCharArray(chipIdA,64);
    chipId_stringB.toCharArray(chipIdB,64);
    chipId_stringC.toCharArray(chipIdC,64);
    chipId_stringD.toCharArray(chipIdD,64);
    chipId_stringE.toCharArray(chipIdE,64);
    #define ledAPin 0
    #define ledBPin 2
    #define ledCPin 15
    #define ledDPin 5
    #define switchAPin 12
    #define switchBPin 13
    #define switchCPin 14
    #define switchDPin 4
    pinMode(ledAPin, OUTPUT);
    pinMode(ledBPin, OUTPUT);
    pinMode(ledCPin, OUTPUT);
    pinMode(ledDPin, OUTPUT);
    pinMode(switchAPin, INPUT);
    pinMode(switchBPin, INPUT);
    pinMode(switchCPin, INPUT);
    pinMode(switchDPin, INPUT);
    digitalWrite(ledAPin, LOW);
    digitalWrite(ledBPin, LOW);
    digitalWrite(ledCPin, LOW);
    digitalWrite(ledDPin, LOW);
    digitalSwitchAState = false;
    digitalSwitchBState = false;
    digitalSwitchCState = false;
    digitalSwitchDState = false;
    switchAState = false;
    switchBState = false;
    switchCState = false;
    switchDState = false;
    oldSwitchAState = false;
    oldSwitchBState = false;
    oldSwitchCState = false;
    oldSwitchDState = false;
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
    currentSwitchAState = digitalRead(switchAPin);
    currentSwitchBState = digitalRead(switchBPin);
    currentSwitchCState = digitalRead(switchCPin);
    currentSwitchDState = digitalRead(switchCPin);
      if(currentSwitchAState != oldSwitchAState){
        lastDebounceATime = millis();
      }else if(currentSwitchBState != oldSwitchBState){
        lastDebounceBTime = millis();
      }else if(currentSwitchCState != oldSwitchCState){
        lastDebounceCTime = millis();
      }else if(currentSwitchDState != oldSwitchDState){
        lastDebounceDTime = millis();
      }
      if ((millis() - lastDebounceATime) > debounceDelay) {
        if (currentSwitchAState != switchAState) {
          switchAState = currentSwitchAState;
          if (switchAState == true) {
          digitalSwitchAState = !digitalSwitchAState;
          StaticJsonBuffer<200> switchABuffer;
          JsonObject& switchAJson = switchABuffer.createObject();
          switchAJson["name"] = chipIdA;
          switchAJson["characteristic"] = "On";
          switchAJson["value"] = digitalSwitchAState;
          String switchAString;
          switchAJson.printTo(switchAString);
          client.publish(outtopic,switchAString);
          }
        }
      }
      if ((millis() - lastDebounceBTime) > debounceDelay) {
        if (currentSwitchBState != switchBState) {
          switchBState = currentSwitchBState;
          if (switchBState == true) {
          digitalSwitchBState = !digitalSwitchBState;
          StaticJsonBuffer<200> switchBBuffer;
          JsonObject& switchBJson = switchBBuffer.createObject();
          switchBJson["name"] = chipIdB;
          switchBJson["characteristic"] = "On";
          switchBJson["value"] = digitalSwitchBState;
          String switchBString;
          switchBJson.printTo(switchBString);
          client.publish(outtopic,switchBString);
          }
        }
      }
      if ((millis() - lastDebounceCTime) > debounceDelay) {
        if (currentSwitchCState != switchCState) {
          switchCState = currentSwitchCState;
          if (switchCState == true) {
          digitalSwitchCState = !digitalSwitchCState;
          StaticJsonBuffer<200> switchCBuffer;
          JsonObject& switchCJson = switchCBuffer.createObject();
          switchCJson["name"] = chipIdC;
          switchCJson["characteristic"] = "On";
          switchCJson["value"] = digitalSwitchCState;
          String switchCString;
          switchCJson.printTo(switchCString);
          client.publish(outtopic,switchCString);
          }
        }
      }
      if ((millis() - lastDebounceDTime) > debounceDelay) {
        if (currentSwitchDState != switchDState) {
          switchDState = currentSwitchDState;
          if (switchCState == true) {
          digitalSwitchDState = !digitalSwitchDState;
          StaticJsonBuffer<200> switchDBuffer;
          JsonObject& switchDJson = switchDBuffer.createObject();
          switchDJson["name"] = chipIdC;
          switchDJson["characteristic"] = "On";
          switchDJson["value"] = digitalSwitchDState;
          String switchDString;
          switchDJson.printTo(switchDString);
          client.publish(outtopic,switchDString);
          }
        }
      }
      oldSwitchAState = currentSwitchAState;
      oldSwitchBState = currentSwitchBState;
      oldSwitchCState = currentSwitchCState;
      oldSwitchDState = currentSwitchDState;
      if(digitalSwitchAState == true){
        digitalWrite(ledAPin, HIGH);
      }else{
        digitalWrite(ledAPin, LOW);
      }
      if(digitalSwitchBState == true){
        digitalWrite(ledBPin, HIGH);
      }else{
        digitalWrite(ledBPin, LOW);
      }
      if(digitalSwitchCState == true){
        digitalWrite(ledCPin, HIGH);
      }else{
        digitalWrite(ledCPin, LOW);
      }
      if(digitalSwitchDState == true){
        digitalWrite(ledDPin, HIGH);
      }else{
        digitalWrite(ledDPin, LOW);
      }
      client.loop();
  } else {
    wifi_conn();
  }
}

void addAccessory(){
    StaticJsonBuffer<200> jsonAddSwitchABuffer;
    JsonObject& addSwitchAJson = jsonAddSwitchABuffer.createObject();
    addSwitchAJson["name"] = chipIdA;
    addSwitchAJson["service"] = "Switch";
    String addSwitchAString;
    addSwitchAJson.printTo(addSwitchAString);
    client.publish(addtopic,addSwitchAString);
    
    StaticJsonBuffer<200> jsonAddSwitchBBuffer;
    JsonObject& addSwitchBJson = jsonAddSwitchBBuffer.createObject();
    addSwitchBJson["name"] = chipIdB;
    addSwitchBJson["service"] = "Switch";
    String addSwitchBString;
    addSwitchBJson.printTo(addSwitchBString);
    client.publish(addtopic,addSwitchBString);

    StaticJsonBuffer<200> jsonAddSwitchCBuffer;
    JsonObject& addSwitchCJson = jsonAddSwitchCBuffer.createObject();
    addSwitchCJson["name"] = chipIdC;
    addSwitchCJson["service"] = "Switch";
    String addSwitchCString;
    addSwitchCJson.printTo(addSwitchCString);
    client.publish(addtopic,addSwitchCString);

    StaticJsonBuffer<200> jsonAddSwitchDBuffer;
    JsonObject& addSwitchDJson = jsonAddSwitchDBuffer.createObject();
    addSwitchDJson["name"] = chipIdD;
    addSwitchDJson["service"] = "Switch";
    String addSwitchDString;
    addSwitchDJson.printTo(addSwitchDString);
    client.publish(addtopic,addSwitchDString);
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
    
    if(accessoryName == std::string(chipIdA)){
      setSwitchJson["name"] = chipIdA;
      digitalSwitchAState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchAState;
      oldSwitchAState = digitalSwitchAState;
    }
    else if(accessoryName == std::string(chipIdB)){
      setSwitchJson["name"] = chipIdB;
      digitalSwitchBState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchBState;
      oldSwitchBState = digitalSwitchBState;
    }
    else if(accessoryName == std::string(chipIdC)){
      setSwitchJson["name"] = chipIdC;
      digitalSwitchCState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchCState;
      oldSwitchCState = digitalSwitchCState;
    }
    else if(accessoryName == std::string(chipIdD)){
      setSwitchJson["name"] = chipIdD;
      digitalSwitchDState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchDState;
      oldSwitchDState = digitalSwitchDState;
    }
    String setSwitchString;
    setSwitchJson.printTo(setSwitchString);
    client.publish(outtopic,setSwitchString);
  }

void getAccessory(){
    StaticJsonBuffer<200> getSwitchBuffer;
    JsonObject& getSwitchJson = getSwitchBuffer.createObject();
    getSwitchJson["name"] = accessoryName;
    getSwitchJson["characteristic"] = "On";
    if(accessoryName == std::string(chipIdA)){
    getSwitchJson["value"] = digitalSwitchAState;
    }else if(accessoryName == std::string(chipIdB)){
      getSwitchJson["value"] = digitalSwitchBState;
    }else if(accessoryName == std::string(chipIdC)){
      getSwitchJson["value"] = digitalSwitchCState;
    }else if(accessoryName == std::string(chipIdD)){
      getSwitchJson["value"] = currentSwitchDState;
    }
    String getSwitchString;
    getSwitchJson.printTo(getSwitchString);
    client.publish(outtopic,getSwitchString);
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
    if (accessoryName == std::string(chipIdA) ||accessoryName == std::string(chipIdB) || accessoryName == std::string(chipIdC) || accessoryName == std::string(chipIdD)){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      switchState = mqttAccessory["value"];
      setAccessory();
    }
  }
    else if (gettopic == std::string(pubTopic)){
      if (accessoryName == std::string(chipId) || accessoryName == std::string(chipIdA) || accessoryName == std::string(chipIdB) || accessoryName == std::string(chipIdC) || accessoryName == std::string(chipIdD) || accessoryName == std::string(chipIdE)){
        accessoryCharacteristic = mqttAccessory["characteristic"];
        getAccessory();
      }
  }
  else if(accessoryName != std::string(chipId)){
    //
  }
}
