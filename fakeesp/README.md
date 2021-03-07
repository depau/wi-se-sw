# Fake ESP build

This directory contains some basic infrastructure and stubs to build and run Wi-Se on Linux without any modifications. This is only intended
for local testing against memory corruption bugs and for debugging with a debugger that actually works.

The code here stubs any Arduino-ESP8266 SDK calls that are used, as well as reimplementing ESPAsyncTCP on top of normal POSIX sockets.

GPIOs are emulated and the up-to-date GPIO status is written to `/tmp/fakeesp_gpio.txt`.

The `delay()`, `delayMicrosecond()` and `yield()` functions, in addition to performing their intended purpose, also provide hooks for the
modified ESPAsyncTCP library in order to emulate the asynchronous callbacks from lwIP.

All the PROGMEM data and strings are turned into `const char *` via preprocessor duct-tape.

## Building

No warranties, but roughly you need to:

- Make sure GCC, CMake and OpenSSL (needed for hash algorithms) are installed
- Make sure you built the ESP8266 version of the project at least once, so that PlatformIO downloads the dependencies
- `mkdir build; cd build; cmake .. && make -j$(nproc)`

Then run `./wi-se_fakeesp`. Make sure you're using a high port in `config.h` for the HTTP server, since we don't have enough privileges for
port 80.

MDNS and ArduinoOTA are completely stubbed, so there's no need to worry about them.

## Licenses

Some of the stub code was written by myself, some of it was copy-pasted and optionally modified.

#### From [esp8266/Arduino](https://github.com/esp8266/Arduino/)

The following files are mostly copy-pasted from the ESP8266-Arduino project. Minor modifications have been made to make them play nicely
with the other stub and to use the fake PROGMEM.

Licensed under the GNU Lesser General Public License v2.1, `./LICENSE.lgpl2`

- `src/cbuf.cpp`
- `include/cbuf.h`
- `include/c_types.h`
- `src/FS.cpp`
- `include/FS.h`
- `include/FSImpl.h`
- `include/Hash.h`
- `src/IPAddress.h`
- `include/IPAddress.h`
- `include/md5.h`
- `src/Print.h`
- `include/Print,h`
- `include/Printable.h`
- `src/Stream.cpp`
- `include/Stream.h`
- `src/WString.cpp`
- `include/WString.h`

The following files also come from the same project, but are released under the public domain instead:

- `src/libb64/`
- `include/libb64/`

#### From [lwIP](https://savannah.nongnu.org/git/?group=lwip)

The following headers were mostly copy-pasted from lwIP with minor modifications.

Licensed under the 3-Clause BSD license, `./LICENSE.bsd`

- `include/lwip/`

#### From [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)

The following files come from ESPAsyncTCP, but have been heavily modified to use POSIX sockets instead of the ESP SDK.

Licensed under the GNU General Public License v3.0, `./ESPAsyncTCP/LICENSE`

- `ESPAsyncTCP/`

#### From scratch

The following files were written from scratch to mimic the API they are stubbing.

Licensed under the main project license, GNU General Public License v3.0, `../LICENSE`

- `main.cpp`
- `src/ArduinoTime.cpp`
- `src/FakeGPIO.cpp`
- `src/GenericStuff.cpp`
- `src/Hash.cpp`
- `src/itoa.cpp`
- `src/md5.cpp`
- `include/Arduino.h`
- `include/ArduinoOTA.h`
- `include/ESP.h`
- `include/ESP8266WiFi.h`
- `include/ESP8266mDNS.h`
- `include/Serial.h` (with some code from Arduino-ESP8266)
- `include/uart.h` (with some code from Arduino-ESP8266)
