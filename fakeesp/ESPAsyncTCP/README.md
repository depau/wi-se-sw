# Fake ESPAsyncTCP

The code in this directory is a fork of ESPAsyncTCP modified to run on Linux using the normal POSIX socket API.

Not everything is implemented and not everything that is implemented works properly.
In particular, only the async server code is implemented. The client, both async and sync, is not implemented.
It was tested with ESPAsyncWebserver, including the WebSocket server handler.

The AsyncServer and AsyncClient classes hook up to callbacks in the fake `Arduino.h`/`ArduinoTime.cpp` as soon as possible.
Any time `delay()` or `yield()` are called from the Arduino application code the AsyncTCP hooks will be called and any callbacks delivered.

Original source code here: https://github.com/me-no-dev/ESPAsyncTCP
Licensed under the GNU General Public License v3.0

Original README below.

----

# ESPAsyncTCP 
[![Build Status](https://travis-ci.org/me-no-dev/ESPAsyncTCP.svg?branch=master)](https://travis-ci.org/me-no-dev/ESPAsyncTCP) ![](https://github.com/me-no-dev/ESPAsyncTCP/workflows/ESP%20Async%20TCP%20CI/badge.svg)

### Async TCP Library for ESP8266 Arduino

For ESP32 look [HERE](https://github.com/me-no-dev/AsyncTCP)

[![Join the chat at https://gitter.im/me-no-dev/ESPAsyncWebServer](https://badges.gitter.im/me-no-dev/ESPAsyncWebServer.svg)](https://gitter.im/me-no-dev/ESPAsyncWebServer?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

This is a fully asynchronous TCP library, aimed at enabling trouble-free, multi-connection network environment for Espressif's ESP8266 MCUs.

This library is the base for [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)

## AsyncClient and AsyncServer
The base classes on which everything else is built. They expose all possible scenarios, but are really raw and require more skills to use.

## AsyncPrinter
This class can be used to send data like any other ```Print``` interface (```Serial``` for example).
The object then can be used outside of the Async callbacks (the loop) and receive asynchronously data using ```onData```. The object can be checked if the underlying ```AsyncClient```is connected, or hook to the ```onDisconnect``` callback.

## AsyncTCPbuffer
This class is really similar to the ```AsyncPrinter```, but it differs in the fact that it can buffer some of the incoming data.

## SyncClient
It is exactly what it sounds like. This is a standard, blocking TCP Client, similar to the one included in ```ESP8266WiFi```

## Libraries and projects that use AsyncTCP
- [ESP Async Web Server](https://github.com/me-no-dev/ESPAsyncWebServer)
- [Async MQTT client](https://github.com/marvinroger/async-mqtt-client)
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)
- [ESP8266 Smart Home](https://github.com/baruch/esp8266_smart_home)
- [KBox Firmware](https://github.com/sarfata/kbox-firmware)
