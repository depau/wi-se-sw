//
// Created by depau on 3/6/21.
//

#ifndef WI_SE_SW_ESP8266MDNS_H
#define WI_SE_SW_ESP8266MDNS_H

#include "IPAddress.h"

class FakeMDNS {
public:
    bool begin(const char *p_pcHostname) { return true; }

    bool begin(const char *p_pcHostname, const IPAddress &p_IPAddress, uint32_t p_u32TTL) { return true; }

    void addService(const char *, const char *, int) {}
};

extern FakeMDNS MDNS;

#endif //WI_SE_SW_ESP8266MDNS_H
