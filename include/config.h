//
// Created by depau on 1/26/21.
//
#include <uart.h>

#ifndef WI_SE_SW_CONFIG_H
#define WI_SE_SW_CONFIG_H

// Enables debug messages to debug UART
#define ENABLE_DEBUG 0
// Prints a bunch of timing measurements to debug UART
#define ENABLE_BENCHMARK 0

// Board configuration
// Available board types:
// - 0: generic ESP8266 boards
// - 1: wi-se-rpi-v0.1
// - 2: wi-se-opi4-v0.1
// - 3: wi-se-rewirable-v0.1
#define BOARD_TYPE 0

// UART configuration
#define UART_COMM Serial
#define UART_COMM_BAUD 1500000
#define UART_COMM_CONFIG (UART_NB_BIT_8 | UART_PARITY_NONE | UART_NB_STOP_BIT_1)

#define UART_DEBUG Serial1
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

// WebSocket configuration
#define WS_MAX_CLIENTS 3
#define WS_PING_INTERVAL 300  // seconds

// Web TTY configuration
// You can specify any option documented here: https://xtermjs.org/docs/api/terminal/interfaces/iterminaloptions/
// Make sure it is a valid JSON and that it's also a valid C string.
#define TTYD_WEB_CONFIG "{\"disableLeaveAlert\": true}"

// LED configuration - only if board type is custom
// Wi-Se board LEDs are pre-configured in wise_boards.h
#if BOARD_TYPE == 0
#define LED_WIFI 2
#define LED_STATUS 2
#define LED_TX 2
#define LED_RX 2
#endif //BOARD_TYPE

#define LED_ON_TIME 15
#define LED_OFF_TIME 25

// End of configuration

#include "wise_boards.h"

#endif // WI_SE_SW_CONFIG_H
