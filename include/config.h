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
#define BOARD_TYPE 1

// UART configuration
#define UART_COMM Serial
#define UART_COMM_BAUD 1500000
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

// OTA updates
#define OTA_ENABLE 1
// You MUST define a password, otherwise OTA won't be enabled.
#define OTA_PASSWORD ""

// Server configuration
#define HTTP_LISTEN_PORT 80
#define HTTP_AUTH_ENABLE false
#define HTTP_AUTH_USER "user"
#define HTTP_AUTH_PASS "password"

// CORS - Uncomment to allow all origins
//#define CORS_ALLOW_ORIGIN "*"

// WebSocket configuration
#define WS_MAX_CLIENTS 3
#define WS_PING_INTERVAL 300  // seconds

// Web TTY configuration
// You can specify any option documented here: https://xtermjs.org/docs/api/terminal/interfaces/iterminaloptions/
// Make sure it is a valid JSON and that it's also a valid C string.
#define TTYD_WEB_CONFIG "{\"disableLeaveAlert\": true}"


#if BOARD_TYPE == 0 // don't change

// LED configuration - only if board type is custom
// Wi-Se board LEDs are pre-configured in wise_boards.h
#define LED_WIFI 2
#define LED_STATUS 2
#define LED_TX 2
#define LED_RX 2

#endif //BOARD_TYPE

// LED timings (milliseconds)
#define LED_ON_TIME 15
#define LED_OFF_TIME 25

// Advanced buffering parameters
// Tweak if you feel brave. Report any improvements, but make sure you test them at 1500000 8N1 and that it works better
// than the defaults before submitting.
// Note that these buffers do not cause measurable latency, they need to be sort of high so that the WebSocket sender
// can catch up as the UART is being stuffed with high speed data.

#define UART_RX_BUF_SIZE 10240
#define UART_RX_SOFT_MIN (WS_SEND_BUF_SIZE * 3 / 2)
#define UART_BUFFER_BELOW_SOFT_MIN_DYNAMIC_DELAY (std::min((int) (1000L * WS_SEND_BUF_SIZE * 8L * 2 / 3 / uartBaudRate), 5))

// UART software flow control, improves stability. It must be supported by the connected device for it to make any
// difference.
#define UART_SW_FLOW_CONTROL 1
#define UART_SW_FLOW_CONTROL_LOW_WATERMARK UART_RX_SOFT_MIN + 1
#define UART_SW_FLOW_CONTROL_HIGH_WATERMARK WS_SEND_BUF_SIZE - 1

#define WS_SEND_BUF_SIZE 1536
#define WS_FRAGMENTED_DATA_BUFFER_SIZE 1536

#define HEAP_FREE_LOW_WATERMARK 4096
#define HEAP_FREE_HIGH_WATERMARK 10240

// If we stopped for half a second and the heap is still stuffed like a turkey we might just as well crash instead of
// continue waiting, this code is probably leaky AF anyway.
#define HEAP_CAUSED_WS_FLOW_CTL_STOP_MAX_MS 500

// End of configuration
#include "wise_boards.h"

#endif // WI_SE_SW_CONFIG_H
