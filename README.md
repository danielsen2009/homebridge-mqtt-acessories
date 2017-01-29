Using homebridge and homebridge-mqtt to relay commands to esp8266 modules from homekit.

HAP-HodeJS: https://github.com/KhaosT/HAP-NodeJS

Homebridge: https://github.com/nfarina/homebridge

Homebridge-mqtt: https://github.com/cflurin/homebridge-mqtt

ArduinoJson: https://github.com/bblanchon/ArduinoJson

ToDo:

! Done

- In Progress

X Postponed

	ALL:
	- Better commenting in sketches.
	- Add last state to eeprom so states survive restart - configurable to on or off.
	- Add first time boot registration and updating availability after booting after the first pairing.

	Switch:
	- Separate get and post functions.
	- Add ability for door lock/ doorbell?
	- Possible web page configuration or grabbing settings from a central location.
	
	Thermostat:
	! Added mainPowerPin, need 4ch relay board and dht11.
	- Build out functionality.
	- Add fan start and stop delay.
	- Add support for 3.5" Nextion Touch Display