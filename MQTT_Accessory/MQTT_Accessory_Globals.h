#ifndef HEADER_GLOBALS
#define HEADER_GLOBALS

#include "Arduino.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>

//Network Settings
const char* ssid = "HomeBridge202";
const char* password = "HomeBridgePassPhrase";
//-----------------------------------------------

char chipId[256];
char chipIdB[256];
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

String chipId_string;
String jsonReachabilityString;

IPAddress server(10, 1, 0, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);
#endif
