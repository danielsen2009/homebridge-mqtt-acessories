/*
  MQTT_Accessory.h - Library for using MQTT to talk to Homebridge.
  Created by Erik Danielsen, February 5, 2017.
  Released into the public domain.
*/

#ifndef MQTT_ACCESSORY_H
#define MQTT_ACCESSORY_H

#include "Arduino.h"

class Outlet{
  
  public:
  Outlet(int outletPin, char outletName[10]);
  char outletId[256];
  char* outletName;
  bool outletState();
  void AddOutlet();
  void SetOutlet();
  void GetOutlet();
  

  private:
  int _outletPin;
  String _outletIdString;
  char _outletId[256];
  char* _outletName;
  bool _outletState;
};

#endif
