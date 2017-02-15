#include "MQTT_Accessory.h"
#include "MQTT_Accessory_Globals.h"


//Declare Accessory Type, Pin and Name
Outlet topOutlet(16, "Top");
Outlet bottomOutlet(14, "Bottom");
//------------------------------------

void setup() {
  Serial.begin(74880);
  String(ESP.getChipId()).toCharArray(chipId,256);
  StaticJsonBuffer<200> jsonReachabilityBuffer;
  JsonObject& jsonReachability = jsonReachabilityBuffer.createObject();
  jsonReachability["name"] = chipId;
  jsonReachability["reachable"] = false;
  jsonReachability.printTo(jsonReachabilityString);
  wifi_conn();
  ArdOTA();
}

void loop() {
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
  topOutlet.AddOutlet();
  bottomOutlet.AddOutlet();
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

void ArdOTA(){
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

void callback(const MQTT::Publish& pub){
  pub.topic().toCharArray(pubTopic,512);
  StaticJsonBuffer<512> jsoncallbackBuffer;
  JsonObject& mqttAccessory = jsoncallbackBuffer.parseObject(pub.payload_string());
  accessoryName = mqttAccessory["name"];
  accessoryServiceName = mqttAccessory["service_name"];
  if(mainttopic == std::string(pubTopic)){
    Serial.print("Maintenance");
    //maintAccessory();
  }
  else if(intopic == std::string(pubTopic)){
    if(accessoryName == std::string(chipId) && (accessoryServiceName == std::string(chipId) || accessoryServiceName == std::string(chipIdB))){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      accessoryValue = mqttAccessory["value"];
      //setAccessory();
    }
  }
  else if(gettopic == std::string(pubTopic)){
    if(accessoryName == std::string(chipId) && (accessoryServiceName == std::string(chipId) || accessoryServiceName == std::string(chipIdB))){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      //getAccessory();
    }
  }
  else if(accessoryName != std::string(chipId)){
    //
  }
}

