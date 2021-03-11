//
// Created by depau on 1/26/21.
//

#ifndef WI_SE_SW_SERVER_H
#define WI_SE_SW_SERVER_H

#include "ttyd.h"
#include "debug.h"
#include "version.h"

#define MASK_UART_PARITY  0B00000011
#define MASK_UART_BITS    0B00001100
#define MASK_UART_STOP    0B00110000

#define WS_CMD_CACHE_EMPTY_SENTINEL 0xFFFFFFFFFFFFFFFF

#define HTTP_AUTH_TOKEN_LEN 16

enum WsCloseReason {
    WS_CLOSE_OK = 1000,
    WS_CLOSE_GOING_AWAY = 1001,
    WS_CLOSE_PROTOCOL_ERROR = 1002,
    WS_CLOSE_DATA_NOT_SUPPORTED = 1003,
    WS_CLOSE_BAD_DATA = 1007,
    WS_CLOSE_POLICY_VIOLATION = 1008,
    WS_CLOSE_TOO_BIG = 1009,
    WS_CLOSE_MISSING_EXTN = 1010,
    WS_CLOSE_BAD_CONDITION = 1011,
};

class WiSeServer {
private:
    // Use larger data type to be able to mark emptyness
    uint64_t wsFragClientCommandCacheClientIds[WS_MAX_CLIENTS] = {WS_CMD_CACHE_EMPTY_SENTINEL};
    uint8_t wsFragClientCommandCache[WS_MAX_CLIENTS] = {0};
    char serverHeader[100] = {0};

public:
    char *token;
    AsyncWebServer *httpd;
    AsyncWebSocket *websocket;
    TTY *ttyd;

    WiSeServer(char *token, AsyncWebServer *httpd, AsyncWebSocket *websocket, TTY *ttyd) :
            token{token},
            httpd{httpd},
            websocket{websocket},
            ttyd{ttyd} {
        // Format server header
        snprintf(serverHeader, sizeof(serverHeader) / sizeof(char), "%s", "Wi-Se/" VERSION);
    };

    void begin();

    void end() const;

    static bool checkHttpBasicAuth(AsyncWebServerRequest *request);

    static void handleIndex(AsyncWebServerRequest *request);

    void handleStatsRequest(AsyncWebServerRequest *request) const;

    void handleSttyRequest(AsyncWebServerRequest *request) const;

    void handleSttyBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) const;

    void sttySendResponse(AsyncWebServerRequest *request) const;

    void handleToken(AsyncWebServerRequest *request) const;


    void
    onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data,
                     size_t len);

private:
    bool garbageCollectCommandCache() {
        bool foundGarbage = false;
        for (unsigned long long clientId : wsFragClientCommandCacheClientIds) {
            if (clientId == WS_CMD_CACHE_EMPTY_SENTINEL) {
                continue;
            }
            if (ttyd->isClientBlocked(clientId) || !ttyd->isClientAuthenticated(clientId)) {
                foundGarbage = true;
                deleteCachedCommand(clientId);
            }
        }
        return foundGarbage;
    }

    int findCommandCacheSlot(uint32_t clientId, bool doGc = true) {
        int firstEmpty = -1;
        for (int i = 0; i < WS_MAX_CLIENTS; i++) {
            if (wsFragClientCommandCacheClientIds[i] == clientId) {
                return i;
            }
            if (firstEmpty == -1 && wsFragClientCommandCacheClientIds[i] == WS_CMD_CACHE_EMPTY_SENTINEL) {
                firstEmpty = i;
            }
        }

        if (firstEmpty == -1 && doGc && garbageCollectCommandCache()) {
            return findCommandCacheSlot(clientId, false);
        }
        if (firstEmpty == -1) {
            debugf("Critical error, all client command caches full\r\n");
            debugf("Client IDs of cached commands: ");
            for(__unused unsigned long long clientDataBufClientId : wsFragClientCommandCacheClientIds) {
                debugf("%llu ", clientDataBufClientId);
            }
            debugf("\r\n");
            panic();
        }

        return firstEmpty;
    }

    char getCachedCommand(uint32_t clientId) {
        int slot = findCommandCacheSlot(clientId);
        if (wsFragClientCommandCacheClientIds[slot] != clientId) {
            debugf("Warning, tried to retrieve command cache for %d but never set!\r\n", clientId);
            return 0;
        }
        return wsFragClientCommandCache[slot];
    }

    void storeCommandCache(uint32_t clientId, char command) {
        int slot = findCommandCacheSlot(clientId);
        wsFragClientCommandCacheClientIds[slot] = clientId;
        wsFragClientCommandCache[slot] = command;
    }

    void deleteCachedCommand(uint32_t clientId) {
        int slot = findCommandCacheSlot(clientId);
        wsFragClientCommandCacheClientIds[slot] = WS_CMD_CACHE_EMPTY_SENTINEL;
        wsFragClientCommandCache[slot] = 0;
    }
};

#endif //WI_SE_SW_SERVER_H
