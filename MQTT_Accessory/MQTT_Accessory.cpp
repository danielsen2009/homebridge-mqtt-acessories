#include "Arduino.h"
#include "MQTT_Accessory.h"

Outlet::Outlet(int pin)
{
  pinMode(pin, OUTPUT);
  _pin = pin;
}

void Outlet::setState()
{
  
}

void Outlet::getState()
{
  
}

