Using homebridge and homebridge-mqtt to relay commands to esp8266 modules from homekit.

HAP-HodeJS: https://github.com/KhaosT/HAP-NodeJS

Homebridge: https://github.com/nfarina/homebridge

Homebridge-mqtt: https://github.com/cflurin/homebridge-mqtt

ToDo:

! Done

- In Progress

X Postponed

	Switch:
	- Add MCP23017 I2c Port Expander (Need more I/O!).
	- Add last state to eeprom so states survive restart - configurable to on or off.
	- Move loop functions so switches and relays work in case of network problems.
	- Add first time boot registration and updating availability after booting after the first pairing.
	X Add ability for door lock/ doorbell?
	- Add support for relays connected to switches.
	- Possible web page configuration or grabbing settings from a central location.
	
	Thermostat:
	 - Build out functionality.
	 - Add support for 3.5" Nextion Touch Display