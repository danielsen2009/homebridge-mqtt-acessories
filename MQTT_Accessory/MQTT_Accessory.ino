#include "MQTT_Accessory.h"

Outlet topOutlet(14);

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  topOutlet.getState();
}
