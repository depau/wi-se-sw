//
// Created by depau on 3/6/21.
//

#ifndef WI_SE_SW_ARDUINO_H
#define WI_SE_SW_ARDUINO_H

//#define SIMULATE_BAUDRATE

#include <cstdint>
#include <cstdarg>
#include <algorithm>

#include "ArduinoTime.h"
#include "ESP.h"
#include "WString.h"
#include "Stream.h"
#include "HardwareSerial.h"
#include "progmem.h"

typedef bool boolean;
#define __unused

// Fake ESP OS functions
#define os_strlen strlen
#define ets_printf(...) fprintf(stderr, __VA_ARGS__)
#define RANDOM_REG32 rand()

#define ADC_MODE(ADC_VCC) void *adc_mode_mock = NULL

#define HIGH 0x1
#define LOW  0x0

#define PWMRANGE 1023

//GPIO FUNCTIONS
#define INPUT             0x00
#define INPUT_PULLUP      0x02
#define INPUT_PULLDOWN_16 0x04 // PULLDOWN only possible for pin16
#define OUTPUT            0x01
#define FAKEMODE_PWM_OUT  0xFF

#define OUTPUT_OPEN_DRAIN 0x03


void pinMode(uint8_t pin, uint8_t mode);

void digitalWrite(uint8_t pin, uint8_t val);

int digitalRead(uint8_t pin);

int analogRead(uint8_t pin);

void analogReference(uint8_t mode);

void analogWrite(uint8_t pin, int val);

void analogWriteFreq(uint32_t freq);

void analogWriteRange(uint32_t range);

void panic();

long map(long x, long in_min, long in_max, long out_min, long out_max);

void setup();

void loop();


// Monkey-patch ESP Async WebServer/src/WebResponses.cpp
namespace std {
    long unsigned int min(long unsigned int n1, unsigned int n2);
}

#endif //WI_SE_SW_ARDUINO_H
