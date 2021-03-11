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
#define CMD_DETECT_BAUD 'B'

// server message
#define CMD_OUTPUT '0'
#define CMD_SET_WINDOW_TITLE '1'
// Defined as a string to be concatenated below
#define CMD_SET_PREFERENCES "2"

#define CMD_SERVER_PAUSE 'S'
#define CMD_SERVER_RESUME 'Q'
#define CMD_SERVER_DETECTED_BAUD 'B'

const char ttydWebConfig[] = CMD_SET_PREFERENCES TTYD_WEB_CONFIG;

#define LED_HANDLE_EVERY_MILLIS 5
#define CLIENT_PING_EVERY_MILLIS (WS_PING_INTERVAL * 1000)
#define CLIENT_TIMEOUT_CHECK_EVERY_MILLIS 10000
#define COLLECT_STATS_EVERY_MILLIS 500

#define CLIENT_TIMEOUT_MILLIS (WS_PING_INTERVAL * 1033)  // WS_PING_INTERVAL * 103.3% == ~310s with a default of 300s

#define FLOW_CTL_SRC_LOCAL  0b01
#define FLOW_CTL_SRC_REMOTE 0b10

#define FLOW_CTL_XOFF 0x13
#define FLOW_CTL_XON 0x11

#define WS_MAX_BLOCKED_CLIENTS 50
#define WS_CLIENT_BLOCK_EXPIRE_MILLIS 5000

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
    uint8_t pendingAuthClients = 0;

    uint8_t wsBlockedClientsLen = 0;
    uint32_t wsBlockedClients[WS_MAX_BLOCKED_CLIENTS] = {0};
    uint64_t wsClientBlockedAtMillis[WS_MAX_BLOCKED_CLIENTS] = {0};

    uint64_t lastClientPingMillis = millis();
    uint64_t lastClientTimeoutCheckMillis = millis();
    uint64_t lastLedHandleMillis = millis();

    union led_blink_request_u requestLedBlink = {{false, false}};
    union led_blink_schedule_u scheduledLedsOffMillis = {{0}};
    union led_blink_schedule_u ledsBusyUntilMillis = {{0}};

    uint8_t uartFlowControlStatus = 0;
    unsigned long uartFlowControlEngagedMillis = 0;

    // We won't take care of flow control commands coming from UART for the time being, it's O(n) but we don't have much
    // time too waste. Also, chances are that it will be handled already by the remote terminal.
    bool wsFlowControlStopped = false;
    unsigned long wsFlowControlEngagedMillis = 0;

    bool pendingBaudDetection = false;
    unsigned long autobaudStartedAtMillis = 0;
    unsigned int autobaudLastAttemptAtMillis = 0;

    // Stats refer to the UART side
    uint64_t lastStatsCollectMillis = millis();
    uint64_t prevTx = 0;
    uint64_t prevRx = 0;
    uint64_t totalTx = 0;
    uint64_t totalRx = 0;

    uint64_t txRate = 0;
    uint64_t rxRate = 0;

public:
    explicit TTY(char *token, AsyncWebSocket *websocket) : token{token}, websocket{websocket} {}

    uint32_t getUartBaudRate() const {
        return uartBaudRate;
    }

    uint8_t getUartConfig() const {
        return uartConfig;
    }

    uint64_t getTotalRx() const { return totalRx; }

    uint64_t getTotalTx() const { return totalTx; }

    uint64_t getRxRate() const { return rxRate; }

    uint64_t getTxRate() const { return txRate; }

    void stty(uint32_t baudrate, uint8_t config);

    bool onNewWebSocketClient(uint32_t clientId);

    void removeClient(uint32_t clientId);

    void handleWebSocketMessage(uint32_t clientId, const uint8_t *buf, size_t len, char fragmentCachedCommand = 0);

    void dispatchUart();

    void performHousekeeping();

    void handleWebSocketPong(uint32_t clientId);

    void shrinkBuffers();

    void blockClient(uint32_t clientId);

    bool isClientBlocked(uint32_t clientId);
    
    bool isClientAuthenticated(uint32_t clientId);

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

    void nukeClient(uint32_t clientId, uint16_t closeReason);

    void sendInitialMessages(uint32_t clientId);

    void sendClientConfiguration(uint32_t clientId);

    size_t snprintWindowTitle(char *dest, size_t len) const;

    void sendWindowTitle(int64_t clientId = -1);

    void flowControlUartRequestStop(uint8_t source);

    void flowControlUartRequestResume(uint8_t source);

    bool wsCanSend();

    bool areAllClientsAuthenticated() const;

    void broadcastBufferToClients(AsyncWebSocketMessageBuffer *wsBuffer);

    void flowControlWebSocketRequest(bool stop);

    bool performFlowControl_SlowWiFi(size_t uartAvailable);

    bool performFlowControl_HeapFull();

    void collectStats();

    void removeExpiredClientBlocks();

    void requestBaudrateDetection();

    void sendBaurateDetectionResult(int64_t bestApprox, int64_t measured);

    void unlockUartFlowControlIfTimedOut();

    void autobaud();
};

#endif //WI_SE_SW_TTYD_H
