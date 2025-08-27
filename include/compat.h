//
// Created by hetii on 8/26/25.
//

#ifndef WI_SE_SW_COMPAT_H
#define WI_SE_SW_COMPAT_H

#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>

static inline String getSdkVersion() {
    return ESP.getFullVersion();
}

static inline uint32_t getChipId() {
    return ESP.getChipId();
}

static inline uint32_t esp_random() {
    return ESP.random();
}

static inline float getVcc(uint8_t pin = 255) {
    (void)pin;
    return ESP.getVcc() / 1000.0;
}

static inline uint8_t getHeapFragmentation() {
    return ESP.getHeapFragmentation();
}

static inline const char* getChipModel() {
    return "ESP8266";
}

#else // ESP32

#include <WiFi.h>
#include "esp_system.h"

static inline void analogWriteRange(uint32_t) {
    return;
}

static inline float getVcc(uint8_t pin = 255) {
    if (pin < 255){
        int raw = analogRead(pin);
        return raw * (3.3 / 4095.0);
    }
    return 0.0;
}

static inline String getSdkVersion() {
    return ESP.getSdkVersion();
}

static inline uint32_t getChipId() {
    return (uint32_t) ESP.getEfuseMac() & 0xFFFFFFFF;
}

static inline uint8_t getHeapFragmentation() {
    size_t free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    if (free == 0) return 0;
    return (uint8_t)((1.0 - (float)largest / free) * 100.0);
}

static inline const char* getChipModel() {
    static char buf[10];
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    switch (chip_info.model) {
        case CHIP_ESP32:    return "ESP32";
        case CHIP_ESP32S2:  return "ESP32-S2";
        case CHIP_ESP32S3:  return "ESP32-S3";
        #ifdef CHIP_ESP32C2
        case CHIP_ESP32C2:  return "ESP32-C2";
        #endif
        case CHIP_ESP32C3:  return "ESP32-C3";
        case CHIP_ESP32H2:  return "ESP32-H2";
        default:
            snprintf(buf, sizeof(buf), "ESP32-M%d", chip_info.model);
            return buf;
    }
}

#endif

#endif // WI_SE_SW_COMPAT_H
