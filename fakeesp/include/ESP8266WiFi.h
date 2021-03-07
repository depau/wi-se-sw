//
// Created by depau on 3/6/21.
//

#ifndef WI_SE_SW_ESP8266WIFI_H
#define WI_SE_SW_ESP8266WIFI_H

#include "lwip/ip_addr.h"
#include "IPAddress.h"

#include <cstdio>

typedef enum {
    WL_NO_SHIELD = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1,
    WL_SCAN_COMPLETED = 2,
    WL_CONNECTED = 3,
    WL_CONNECT_FAILED = 4,
    WL_CONNECTION_LOST = 5,
    WL_DISCONNECTED = 6
} wl_status_t;

typedef enum WiFiMode {
    WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3,
    /* these two pseudo modes are experimental: */ WIFI_SHUTDOWN = 4, WIFI_RESUME = 8
} WiFiMode_t;

typedef enum WiFiPhyMode {
    WIFI_PHY_MODE_11B = 1, WIFI_PHY_MODE_11G = 2, WIFI_PHY_MODE_11N = 3
} WiFiPhyMode_t;

typedef enum WiFiSleepType {
    WIFI_NONE_SLEEP = 0, WIFI_LIGHT_SLEEP = 1, WIFI_MODEM_SLEEP = 2
} WiFiSleepType_t;

struct station_config {
};


struct WiFiState
{
    uint32_t crc;
    struct
    {
        station_config fwconfig;
        ip_info ip;
        ip_addr_t dns[2];
        ip_addr_t ntp[2];
        WiFiMode_t mode;
        uint8_t channel;
        bool persistent;
    } state;
};



class FakeWiFi {
public:
    wl_status_t begin() { return WL_CONNECTED; };

    wl_status_t begin(const char *ssid) { return WL_CONNECTED; };

    wl_status_t begin(const char *ssid, const char *passphrase) { return WL_CONNECTED; };

    wl_status_t begin(const char *ssid, const char *passphrase, int32_t channel, const uint8_t *bssid,
                             bool connect) { return WL_CONNECTED; };

    wl_status_t status() { return WL_CONNECTED; };

    void setOutputPower(float dBm) {};

    bool setPhyMode(WiFiPhyMode_t mode) { return true; };

    bool setSleepMode(WiFiSleepType_t type) { return true; };

    bool setSleepMode(WiFiSleepType_t type, uint8_t listenInterval) { return true; };


    bool mode(WiFiMode_t m) { return true; }

    bool mode(WiFiMode_t m, WiFiState* state) { return true; }

    bool hostname(const char* aHostname) { return true; };

    bool softAP(const char* ssid, const char* passphrase, int channel, int ssid_hidden, int max_connection) { return true; }

    String SSID() const {
        return String("LollerinoWiFi");
    }

    String BSSIDstr() {
        return String("de:ad:be:ef:00");
    }

    String macAddress() {
        return String("f0:0d:ba:be:00");
    }

    IPAddress localIP() {
        fprintf(stderr, "STUB WiFi.localIP()");
        return IPAddress(0x01010101);
    }

    IPAddress subnetMask() {
        fprintf(stderr, "STUB WiFi.subnetMask()");
        return IPAddress(0x00FFFFFF);
    }

    IPAddress gatewayIP() {
        fprintf(stderr, "STUB WiFi.gatewayIP()");
        return IPAddress(0x09090909);
    }

    int32_t RSSI() {
        return 42;
    }
};

extern FakeWiFi WiFi;

#endif //WI_SE_SW_ESP8266WIFI_H
