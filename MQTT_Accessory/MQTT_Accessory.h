/*
  MQTT_Accessory.h - Library for using MQTT to talk to Homebridge.
  Created by Erik Danielsen, February 5, 2017.
  Released into the public domain.
*/

#ifndef MQTT_ACCESSORY_H
#define MQTT_ACCESSORY_H

#include "Arduino.h"


class Outlet
{
  public:
    Outlet(int pin);
    void getState();
    void setState();

  private:
    int _pin;
};

#endif
