//
// Created by depau on 1/27/21.
//

#ifndef WI_SE_SW_TTYD_H
#define WI_SE_SW_TTYD_H

#include <AsyncWebSocket.h>
#include "config.h"

// client message
#define CMD_INPUT '0'
#define CMD_RESIZE_TERMINAL '1'
#define CMD_PAUSE '2'
#define CMD_RESUME '3'
#define CMD_JSON_DATA '{'

// server message
#define CMD_OUTPUT '0'
#define CMD_SET_WINDOW_TITLE '1'
// Defined as a string to be concatenated below
#define CMD_SET_PREFERENCES "2"

const char ttydWebConfig[] = CMD_SET_PREFERENCES TTYD_WEB_CONFIG;

#define LED_HANDLE_EVERY_MILLIS 5
#define CLIENT_PING_EVERY_MILLIS (WS_PING_INTERVAL * 1000)
#define CLIENT_TIMEOUT_CHECK_EVERY_MILLIS 10000

#define CLIENT_TIMEOUT_MILLIS (WS_PING_INTERVAL * 1033)  // WS_PING_INTERVAL * 103.3% == ~310s with a default of 300s

struct led_blink_request_t {
    bool rx;
    bool tx;
};

union led_blink_request_u {
    struct led_blink_request_t leds;
    bool array[2];
};

struct led_blink_schedule_t {
    uint64_t rx;
    uint64_t tx;
};

union led_blink_schedule_u {
    struct led_blink_schedule_t leds;
    uint64_t array[2];
};


class TTY {
private:
    char *token;
    AsyncWebSocket *websocket;

    uint32_t uartBaudRate = UART_COMM_BAUD;
    uint8_t uartConfig = UART_COMM_CONFIG;
    bool uartBegun = false;

    uint8_t wsClientsLen = 0;
    uint32_t wsClients[WS_MAX_CLIENTS] = {0};
    uint64_t wsClientsLastSeen[WS_MAX_CLIENTS] = {0};

    uint64_t lastClientPingMillis = millis();
    uint64_t lastClientTimeoutCheckMillis = millis();
    uint64_t lastLedHandleMillis = millis();

    union led_blink_request_u requestLedBlink = {{false, false}};
    union led_blink_schedule_u scheduledLedsOffMillis = {{0}};
    union led_blink_schedule_u ledsBusyUntilMillis = {{0}};

public:
    explicit TTY(char *token, AsyncWebSocket *websocket) : token{token}, websocket{websocket} {}

    uint32_t getUartBaudRate() const {
        return uartBaudRate;
    }

    uint8_t getUartConfig() const {
        return uartConfig;
    }

    void stty(uint32_t baudrate, uint8_t config);

    bool canHandleClient(uint32_t clientId) const;

    void removeClient(uint32_t clientId);

    void handleWebSocketMessage(uint32_t clientId, const uint8_t *buf, size_t len);

    void dispatchUart();

    void performHousekeeping();

    void handleWebSocketPong(uint32_t clientId);

private:

    int findClientIndex(uint32_t clientId) {
        for (int i = 0; i < wsClientsLen; i++) {
            if (wsClients[i] == clientId) {
                return i;
            }
        }
        return -1;
    }

    void clientSeen(uint32_t clientId) {
        int i = findClientIndex(clientId);
        if (i >= 0) {
            wsClientsLastSeen[i] = millis();
        }
    }

    void pingClients();

    void checkClientTimeouts();

    void handleLedBlinkRequests();

    void markClientAuthenticated(uint32_t clientId);

    bool isClientAuthenticated(uint32_t clientId);

    void nukeClient(uint32_t clientId, uint16_t closeReason);

    void broadcastBufferToClients(uint8_t *buf, size_t len);

    void sendInitialMessages(uint32_t clientId);

    void sendClientConfiguration(uint32_t clientId);

    size_t snprintWindowTitle(char *dest, size_t len) const;

    void sendWindowTitle(uint32_t clientId = -1);
};

#endif //WI_SE_SW_TTYD_H