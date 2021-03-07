//
// Created by depau on 3/6/21.
//

#ifndef WI_SE_SW_ESP_H
#define WI_SE_SW_ESP_H

#include <stdint.h>
#include <stdio.h>
#include <algorithm>
#include "WString.h"

class FakeESP {
public:
    uint32_t getFreeHeap() { return 999999; }
    uint16_t getVcc() { return 3300; }
    String getFullVersion() {
        return String("Totally an ESP8266");
    }
    uint32_t getChipId() {
        return 0xDEADBEEF;
    }
    uint32_t random() {
        return rand(); // NOLINT(cert-msc50-cpp)
    }
    void reset() {
        fprintf(stderr, "ESP.reset()\n");
        exit(0);
    }
    void restart() {
        fprintf(stderr, "ESP.restart()\n");
        exit(0);
    }
    uint8_t getCpuFreqMHz() const {
        return 69;
    }

    uint8_t getHeapFragmentation() {
        return 1;
    }
};

extern FakeESP ESP;

#endif //WI_SE_SW_ESP_H
