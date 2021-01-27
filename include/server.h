//
// Created by depau on 1/26/21.
//

#ifndef WI_SE_SW_SERVER_H
#define WI_SE_SW_SERVER_H

#include "ttyd_proto.h"

#define MASK_UART_PARITY  0B00000011
#define MASK_UART_BITS    0B00001100
#define MASK_UART_STOP    0B00110000

class WiSeServer {
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

    static bool checkHttpBasicAuth(AsyncWebServerRequest *request);

    static void handleIndex(AsyncWebServerRequest *request);

    void handleStty(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) const;

    void handleToken(AsyncWebServerRequest *request) const;

    void start();

    void getStty(AsyncWebServerRequest *request) const;
};

#endif //WI_SE_SW_SERVER_H
