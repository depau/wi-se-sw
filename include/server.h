//
// Created by depau on 1/26/21.
//

#ifndef WI_SE_SW_SERVER_H
#define WI_SE_SW_SERVER_H

#include "ttyd.h"
#include "debug.h"

#define MASK_UART_PARITY  0B00000011
#define MASK_UART_BITS    0B00001100
#define MASK_UART_STOP    0B00110000

#define WS_FRAGMENTED_DATA_BUFFER_SIZE 1536
#define WS_DATA_BUF_EMPTY_SENTINEL 0xFFFFFFFFFFFFFFFF

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
    uint64_t clientDataBufClientIds[WS_MAX_CLIENTS] = {WS_DATA_BUF_EMPTY_SENTINEL};
    uint32_t clientDataBufLens[WS_MAX_CLIENTS] = {0};
    uint8_t *clientDataBuffers[WS_MAX_CLIENTS] = {0};

public:
    char *token;
    AsyncWebServer *httpd;
    AsyncWebSocket *websocket;
    TTY *ttyd;

    WiSeServer(char *token, AsyncWebServer *httpd, AsyncWebSocket *websocket, TTY *ttyd) :
            token{token},
            httpd{httpd},
            websocket{websocket},
            ttyd{ttyd} {};

    void begin();

    static bool checkHttpBasicAuth(AsyncWebServerRequest *request);

    static void handleIndex(AsyncWebServerRequest *request);

    void handleSttyRequest(AsyncWebServerRequest *request) const;

    void handleSttyBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) const;

    void sttySendResponse(AsyncWebServerRequest *request) const;

    void handleToken(AsyncWebServerRequest *request) const;


    void
    onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data,
                     size_t len);

private:
    int findDataBufferForClient(uint32_t clientId) {
        int firstEmpty = -1;
        for (int i = 0; i < WS_MAX_CLIENTS; i++) {
            if (clientDataBufClientIds[i] == WS_DATA_BUF_EMPTY_SENTINEL && firstEmpty == -1) {
                firstEmpty = i;
            }
            if (clientDataBufClientIds[i] == clientId) {
                debugf("Found buffer for %d at pos %d\r\n", clientId, i);
                return i;
            }
        }
        debugf("Allocating new buffer for %d at pos %d\r\n", clientId, firstEmpty);
        clientDataBuffers[firstEmpty] = static_cast<uint8_t *>(malloc(WS_FRAGMENTED_DATA_BUFFER_SIZE));
        clientDataBufClientIds[firstEmpty] = clientId;
        return firstEmpty;
    }

    void deallocClientDataBuffer(int pos) {
        if (clientDataBuffers[pos] != nullptr) {
            debugf("Freeing buffer for %lld at pos %d, pointer is %p\n", clientDataBufClientIds[pos], pos, clientDataBuffers[pos]);
            free(clientDataBuffers[pos]);
            clientDataBuffers[pos] = nullptr;
        }
        clientDataBufClientIds[pos] = WS_DATA_BUF_EMPTY_SENTINEL;
        clientDataBufLens[pos] = 0;
    }
};

#endif //WI_SE_SW_SERVER_H
