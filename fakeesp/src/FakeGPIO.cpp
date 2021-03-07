//
// Created by depau on 3/6/21.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Arduino.h"

#define FAKEESP_ENU_PINS 17

char fakeEspDefaultGpioFilePath[] = "/tmp/fakeesp_gpio.txt";

uint8_t pinModes[FAKEESP_ENU_PINS] = {42}; // Init to invalid pin mode
int pinValues[FAKEESP_ENU_PINS] = {0};


char *getGpioFilePath() {
    char *path = getenv("FAKEESP_GPIO_FILE");
    if (path == NULL) {
        return fakeEspDefaultGpioFilePath;
    }
    return path;
}

void writePinFile() {
    FILE *file = fopen(getGpioFilePath(), "w");
    if (file == NULL) {
        fprintf(stderr, "Unable to open fake GPIO file (%d): %s\n", errno, strerror(errno));
        return;
    }
    fprintf(file, "PIN:  ");
    for (int i = 0; i < FAKEESP_ENU_PINS; i++) {
        fprintf(file, " %3d ", i);
    }
    fprintf(file, "\n------");
    for (int i = 0; i < FAKEESP_ENU_PINS; i++) {
        fprintf(file, "-----");
    }
    fprintf(file, "\nMODE: ");
    for (int i = 0; i < FAKEESP_ENU_PINS; i++) {
        if (pinModes[i] == OUTPUT) {
            fprintf(file, " OUT ");
        } else if (pinModes[i] == INPUT) {
            fprintf(file, " INP ");
        } else if (pinModes[i] == FAKEMODE_PWM_OUT) {
            fprintf(file, " PWM ");
        } else {
            fprintf(file, " ??? ");
        }
    }
    fprintf(file, "\nVALUE:");
    for (int i = 0; i < FAKEESP_ENU_PINS; i++) {
        fprintf(file, " %3d ", pinValues[i]);
    }
    fputc('\n', file);
    fclose(file);
}

void pinMode(uint8_t pin, uint8_t mode) {
    callOnDelayCallbacks();
    pinModes[pin] = mode;
}

void digitalWrite(uint8_t pin, uint8_t val) {
    callOnDelayCallbacks();
    if (pinModes[pin] == FAKEMODE_PWM_OUT) {
        pinModes[pin] = OUTPUT;
    }
    pinValues[pin] = val != 0;
    writePinFile();
}

int digitalRead(uint8_t pin) {
    callOnDelayCallbacks();
    return pinValues[pin] != 0;
}

int analogRead(uint8_t pin) {
    callOnDelayCallbacks();
    return pinValues[pin];
}

void analogReference(uint8_t mode) {}

void analogWrite(uint8_t pin, int val) {
    callOnDelayCallbacks();
    pinModes[pin] = FAKEMODE_PWM_OUT;
    pinValues[pin] = val;
    writePinFile();
}

void analogWriteFreq(uint32_t freq) {}

void analogWriteRange(uint32_t range) {}

