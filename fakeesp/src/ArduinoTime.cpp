//
// Created by depau on 3/6/21.
//

#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#include "Arduino.h"

#define ODJT_LEN 50

#ifndef NULL
#define NULL 0
#endif

void (*onDelayJumpTable[ODJT_LEN])(void *arg) = {0};
void *onDelayArgs[ODJT_LEN] = {0};


int32_t registerOnDelayCallback(void (*callback)(void *arg), void *arg) {
    for (int32_t i = 0; i < ODJT_LEN; i++) {
        if (onDelayJumpTable[i] == NULL) {
            onDelayJumpTable[i] = callback;
            onDelayArgs[i] = arg;
            return i;
        }
    }
    return -1;
}

void deregisterOnDelayCallback(uint32_t id) {
    onDelayJumpTable[id] = nullptr;
    onDelayArgs[id] = nullptr;
}

void callOnDelayCallbacks() {
    for (int i = 0; i < ODJT_LEN; i++) {
        if (onDelayJumpTable[i] != nullptr) {
            (*onDelayJumpTable[i])(onDelayArgs[i]);
        }
    }
}

void yield() {
    delay(1);
}

unsigned long millis() {
    return micros64() / 1000;
}

unsigned long micros() {
    return micros64();
}

uint64_t micros64() {
    struct timeval tv = {0};
    gettimeofday(&tv, nullptr);
    return (1000000 * tv.tv_sec) + tv.tv_usec;
}

void delay(unsigned long ms) {
    callOnDelayCallbacks();
    delayMicroseconds(ms * 1000);
}

void delayMicroseconds(unsigned int us) {
    callOnDelayCallbacks();
    struct timespec delta = {us / (1000 * 1000), (us % (1000 * 1000)) * 1000};
    while (nanosleep(&delta, &delta));
}