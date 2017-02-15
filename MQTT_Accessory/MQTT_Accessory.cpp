/*
  MQTT_Accessory.cpp - Library for using MQTT to talk to Homebridge.
  Created by Erik Danielsen, February 5, 2017.
  Released into the public domain.
*/

#include "Arduino.h"
#include "MQTT_Accessory.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

Outlet::Outlet(int outletPin, char outletName[10]){
  Serial.begin(74880);
  _outletPin = outletPin;
  _outletName = outletName;
  _outletState = false;
  _outletIdString = String(_outletName)+"Outlet"+String(ESP.getChipId());
  _outletIdString.toCharArray(_outletId, 256);
  pinMode(_outletPin, OUTPUT);
  digitalWrite(_outletPin, HIGH);
}

void Outlet::AddOutlet(){
  Serial.println(_outletId);
  outletId == _outletId;
}

