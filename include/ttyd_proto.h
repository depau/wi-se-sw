//
// Created by depau on 1/27/21.
//

#ifndef WI_SE_SW_TTYD_PROTO_H
#define WI_SE_SW_TTYD_PROTO_H

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
#define CMD_SET_PREFERENCES '2'

class TTY {
private:
    char *token;
    AsyncWebSocket *websocket;

    uint32_t uartBaudRate = UART_COMM_BAUD;
    uint8_t uartConfig = UART_COMM_CONFIG;
    uint32_t wsClients[32] = {};
    uint8_t wsClientsLen = 0;

public:
    explicit TTY(char *token, AsyncWebSocket *websocket) : token{token}, websocket{websocket} {}

    uint32_t getUartBaudRate() const {
        return uartBaudRate;
    }

    uint8_t getUartConfig() const {
        return uartConfig;
    }

    void stty(uint32_t baudrate, uint8_t config);
};


#endif //WI_SE_SW_TTYD_PROTO_H
