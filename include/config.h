//
// Created by depau on 1/26/21.
//
#include <uart.h>

#ifndef WI_SE_SW_CONFIG_H
#define WI_SE_SW_CONFIG_H

// Board configuration
// Available board types:
// - 0: generic ESP8266 boards
// - 1: wi-se-rpi-v0.1
// - 2: wi-se-opi4-v0.1
// - 3: wi-se-rewirable-v0.1
#define BOARD_TYPE 0

// UART configuration
#define UART_COMM Serial
#define UART_COMM_BAUD 115200
#define UART_COMM_CONFIG (UART_NB_BIT_8 | UART_PARITY_NONE | UART_NB_STOP_BIT_1)

#define UART_DEBUG Serial
#define UART_DEBUG_BAUD 115200
#define UART_DEBUG_CONFIG (UART_NB_BIT_8 | UART_PARITY_NONE | UART_NB_STOP_BIT_1)

// Wi-Fi configuration
// WIFI_STA for client (station) mode
// WIFI_AP for Access Point mode
#define WIFI_MODE WIFI_STA

#define WIFI_SSID "Network name"
#define WIFI_PASS "password"
#define WIFI_HOSTNAME "Wi_Se"

// Access Point configuration
#define WIFI_CHANNEL 6
#define WIFI_HIDE_SSID false
#define WIFI_MAX_DEVICES 8

// Server configuration
#define HTTP_LISTEN_PORT 80
#define HTTP_AUTH_ENABLE false
#define HTTP_AUTH_USER "user"
#define HTTP_AUTH_PASS "password"

// LED configuration - only if board type is custom
// Wi-Se board LEDs are pre-configured in wise_boards.h
#if BOARD_TYPE == 0
#define LED_WIFI 2
#define LED_STATUS 2
#define LED_TX 2
#define LED_RX 2
#endif //BOARD_TYPE

#include "wise_boards.h"

#endif // WI_SE_SW_CONFIG_H
