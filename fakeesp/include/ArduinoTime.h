//
// Created by depau on 3/8/21.
//

#ifndef WI_SE_SW_ARDUINOTIME_H
#define WI_SE_SW_ARDUINOTIME_H

unsigned long millis();

unsigned long micros();

uint64_t micros64();

void delay(unsigned long);

void delayNoYield(unsigned long);

void delayMicroseconds(unsigned int us);

void delayMicrosecondsNoYield(unsigned int us);

void yield();

int32_t registerOnDelayCallback(void (*callback)(void *arg), void *arg);

void deregisterOnDelayCallback(uint32_t id);

void callOnDelayCallbacks();

#endif //WI_SE_SW_ARDUINOTIME_H
