Using homebridge and homebridge-mqtt to relay commands to esp8266 modules from homekit.

HAP-HodeJS: https://github.com/KhaosT/HAP-NodeJS

Homebridge: https://github.com/nfarina/homebridge

Homebridge-mqtt: https://github.com/cflurin/homebridge-mqtt

ArduinoJson: https://github.com/bblanchon/ArduinoJson
_____________________________________________________

Trying to build a library so you can just set the device type and set variables specific to that type. Any insight would be appreciated!
_____________________________________________________

Very little configuration required for homebridge, just get mosquitto, homebridge, homebridge-mqtt running and then change your ssid(HomeBridge), pass(HomeBridgePassPhrase) and ip address(10.1.0.1) and you're all set! 
The esp8266s generate their own unique device ids based on the chip id + service. 
They show up in arduino ide for OTA updates.
_____________________________________________________

ToDo:

! Done

- In Progress

X Postponed

# Comment

	ALL:
	- Better commenting in sketches.
	- Add last state to eeprom so states survive restart - configurable to on or off.
	- Add first time boot registration and updating availability after booting after the first pairing.
	- Security!
	- Add periodic "keepalive" messages? (Availability=true)
	
	Lightbulb:
	- Add ability to set lightbulb type and number of leds. (on/off, dimmable, rgb led)
	- Add ability for manual override, turn leds off without turning esp off.

	Fan:
	# There are a few different options for fans, on/off, speed and rotation direction.
	- Update fan sketch to comply with homebridge-* updates.
	- Have fan-type and service-type separate so you can change it to fit your fan type easily.

	Switch:
	- Update switch sketch to comply with homebridge-* updates.
	- Separate get and post functions.
	X Add ability for deadbolt.
	X Add ability for doorbell.
	
	Thermostat:
	! Added mainPowerPin, need 4ch relay board and dht11/22.
	- Build out functionality.
	- Add fan start and stop delay.
	X Add support for 3.5" Nextion Touch Display (Still planned)