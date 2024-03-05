ESPHome Simplebus2
===================

The ``simplebus2`` component allows you to connect your [Simpebus2 bridge](https://github.com/Elektroarzt/simplebus2-mqtt-bridge) to Home Assistant via de native API. Inspired by both the original Simplebus2 MQTT bridge code and [comelit-esphome](https://github.com/mansellrace/comelit-esphome) (read: mashed the projects together so I could have the bridge running ESPHome)

How to use:

```
    external_components:
      - source: github://se-bastiaan/esphome-simplebus2
      
    simplebus2:
    
	binary_sensor:
	  - platform: simplebus2
	    address: 42
	button:
	  - platform: template
	    name: Open Door
	    on_press:
	      - simplebus2.send:
	          command: 16
	          address: 42
```


Configuration variables:
------------------------

- **gain** (int): See [Simpebus2 bridge](https://github.com/Elektroarzt/simplebus2-mqtt-bridge) documentation
- **voltage_level** (int): See [Simpebus2 bridge](https://github.com/Elektroarzt/simplebus2-mqtt-bridge) documentation
- **rx_pin** (*Optional*, pin): You do not need this as it is the correct pin for the bridge by default
- **tx_pin** (*Optional*, pin): You do not need this as it is the correct pin for the bridge by default
- **filter** (_Optional_,  [Time](https://esphome.io/guides/configuration-types#config-time)): Filter any pulses that are shorter than this. Useful for removing glitches from noisy signals. Defaults to `1000us`. 
-   **idle**  (_Optional_,  [Time](https://esphome.io/guides/configuration-types#config-time)): The amount of time that a signal should remain stable (i.e. not change) for it to be considered complete. Defaults to  `10ms`.
- <a id="eventlist">**event**</a>  (_Optional_, string): The name of the event that will be generated on Home Assistant when receiving a command on the bus. For example, if  set to `simplebus2`, the event generated will be "esphome.simplebus2".
Read more about how to use it in the [event section](#event)
Default to `simplebus2`.
If this parameter is set to `none` no event will be generated.


Binary sensor
===================

You can configure binary sensors will trigger when a particular combination of command and address is received from the bus.

You can also set only the address, in this case the default command is 50, which occurs when a call is made from the outside intercom to the inside intercom.

Configuration examples:

	binary_sensor:
	  - platform: simplebus2
	    address: 16
	  - platform: simplebus2
	    command: 29
	    address: 1
	    name: Internal Door opened
	    auto_off: 60s

- **address** (**Required**, int): The address that when received sets the sensor to on .
- **command** (*Optional*, int): The command that when received sets the sensor to on . Defaults to  `50`.
- **auto_off** (*Optional*,  [Time](https://esphome.io/guides/configuration-types#config-time)):  The time after which the sensor returns to off. If set to `0s` the sensor once it goes on, it stays there until it is turned off by an automation. Defaults to  `30s`.
- **icon** (*Optional*, icon): Manually set the icon to use for the sensor in the frontend. Default to `mdi:doorbell`.
- **id** (*Optional*, string): Manually specify the ID for code generation.
- **name** (*Optional*, string): The name for the sensor. Default to `Incoming call`.

    Note:
    If you have friendly_name set for your device and you want 
    the sensor to use that name, you can set `name: None`.

Event
========
If the [event](#eventlist) parameter is not set to `none`, an event will be generated each time a command is received.

You can intercept events in Home Assistant on the page "developer tools -> event"

Each time a command is received, an event like this will be generated:

	event_type: esphome.simplebus2
	data:
	  device_id: xxxxxxxxxxxxxxxxxxxxxxxxx
	  address: "13"
	  command: "50"
	origin: LOCAL
	time_fired: "2024-01-01T00:00:00.000000+00:00"
	context:
	  id: xxxxxxxxxxxxxxxxxxxxxxxx
	  parent_id: null
	  user_id: null

To intercept this event to trigger an Home Assistant automation, you can use a trigger of type "event."

The trigger configuration will look like this:

	platform: event
	event_type: esphome.simplebus2
	event_data:
	  command: "50"
	  address: "13"
You have to change the address and the name of the event you have chosen, if you have set a different one.

Send a command
==================
To send commands to the bus, the following action is available:

	- simplebus2.send:
	    command: 16
	    address: 1

- **command** (**Required**, int)
- **address** (**Required**, int)

### Button:
The action can be easily inserted into a button type entity:

	button:
	  - platform: template
	    name: Open Door
	    on_press:
	      - simplebus2.send:
	          command: 16
	          address: 5

### Service:
You can create a Home Assistant service, which can easily be invoked by an automation or script:

	api:
	  encryption:
	    key: "xxxxxxxxxxxxxxxxxxx"
	  services:
	    - service: open_door
	      then:
	        - simplebus2.send:
	            command: 16
	            address: 5

### Sending multiple commands:
There are some special configurations that require sending 2 or more commands consecutively on the bus.
In this case, a delay of at least 200ms must be inserted between the commands (one command takes about 180ms to be sent)

	- simplebus2.send:
	    command: 29
	    address: 1
	- delay: 200ms
	- simplebus2.send:
	    command: 16
	    address: 1
