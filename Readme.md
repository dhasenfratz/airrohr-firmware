[![Build Status](https://travis-ci.org/dhasenfratz/airrohr-firmware.svg?branch=master)](https://travis-ci.org/dhasenfratz/airrohr-firmware)

Airrohr Firmware
================
Strip down firmware based on version `NRZ-2017-100` of the [luftdaten.info](http://luftdaten.info) airrohr-firmware:

https://github.com/opendata-stuttgart/sensors-software/tree/master/airrohr-firmware


## Wiring

* SDS and DHT wiring: [https://raw.githubusercontent.com/opendata-stuttgart/meta/master/files/nodemcu-v3-schaltplan-sds011.jpg](https://raw.githubusercontent.com/opendata-stuttgart/meta/master/files/nodemcu-v3-schaltplan-sds011.jpg)

## Tools

* [Arduino IDE](https://www.arduino.cc/en/Main/Software)
* [ESP8266 für Arduino](http://arduino.esp8266.com/stable/package_esp8266com_index.json)

### Arduino IDE Settings

* Board: NodeMCU 1.0 (ESP-12E Module)
* CPU Frequency: 80MHz
* Flash Size: 4M (3M SPIFFS)

## Dependencies
The project has the following external dependencies:

* ArduinoJson@5.8.4
* DHT sensor library@1.3.0
* EspSoftwareSerial@3.3.1
* Adafruit Unified Sensor@1.0.2

## Wiring

### SDS011
* Pin 1 (TX)   &rarr; Pin D1 (GPIO5)
* Pin 2 (RX)   &rarr; Pin D2 (GPIO4)
* Pin 3 (GND)  &rarr; GND
* Pin 4 (2.5m) &rarr; unused
* Pin 5 (5V)   &rarr; VU
* Pin 6 (1m)   &rarr; unused

### DHT22
* Pin 1 &rarr; 3V3
* Pin 2 &rarr; Pin D7 (GPIO13)
* Pin 3 &rarr; unused
* Pin 4 &rarr; GND

### Luftdaten.info API Pins
The luftdaten.info API needs for each sample type a specific pin number to correctly match the measurements:

* DHT22 &rarr; Pin 7
* SDS011 &rarr; Pin 1
