//
// Created by depau on 3/6/21.
//

#include <stdlib.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "Arduino.h"

void panic() {
    fflush(stderr);
    fflush(stdout);
    exit(1);
}


long map(long x, long in_min, long in_max, long out_min, long out_max) {
    const long dividend = out_max - out_min;
    const long divisor = in_max - in_min;
    const long delta = x - in_min;

    return (delta * dividend + (divisor / 2)) / divisor + out_min;
}

// Monkey-patch ESP Async WebServer/src/WebResponses.cpp
namespace std { // NOLINT(cert-dcl58-cpp)
    long unsigned int min(long unsigned int n1, unsigned int n2) {
        return min(n1, (long unsigned int) n2);
    }
}



FakeESP ESP;
HardwareSerial Serial(stdout);
HardwareSerial Serial1(stderr);
FakeArduinoOTA ArduinoOTA;
FakeWiFi WiFi;
FakeMDNS MDNS;