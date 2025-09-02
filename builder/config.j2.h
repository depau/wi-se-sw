{# This file generates the config.h header. This is the file you need to edit if you would like to make changes #}
//
// {{ cfg.AUTOGEN_MSG }}
// {{ cfg.AUTOGEN_DATE }}
//

#include <uart.h>

#ifndef WI_SE_SW_CONFIG_H
#define WI_SE_SW_CONFIG_H

// Enables debug messages to debug UART.
#define ENABLE_DEBUG {{ cfg.ENABLE_DEBUG }}

// Prints a bunch of timing measurements to debug UART.
#define ENABLE_BENCHMARK {{ cfg.ENABLE_BENCHMARK }}

// Available board types:
// - 0: generic ESP8266 boards
// - 1: wi-se-rpi-v0.1
// - 2: wi-se-opi4-v0.1
// - 3: wi-se-rewirable-v0.1
// - 4: generic ESP32 boards
#define BOARD_TYPE {{ cfg.BOARD_TYPE }}

// Optional gpio used by ESP32 to probe vcc.
#define ADC_INPUT {{ cfg.ADC_INPUT }}

// UART configuration:
#define UART_COMM {{ cfg.UART_COMM }}
#define UART_COMM_BAUD {{ cfg.UART_COMM_BAUD }}
#define UART_COMM_CONFIG {{ cfg.UART_COMM_CONFIG }}
#define UART_COMM_TX_EN {{ cfg.UART_COMM_TX_EN }}

#define UART_DEBUG {{ cfg.UART_DEBUG }}
#define UART_DEBUG_BAUD {{ cfg.UART_DEBUG_BAUD }}
#define UART_DEBUG_CONFIG {{ cfg.UART_DEBUG_CONFIG }}

// Wi-Fi configuration:
// WIFI_STA for client (station) mode.
// WIFI_AP for Access Point mode.
#define WIFI_MODE {{ cfg.WIFI_MODE }}

#define WIFI_SSID {{ cfg.WIFI_SSID }}
#define WIFI_PASS {{ cfg.WIFI_PASS }}
#define WIFI_HOSTNAME {{ cfg.WIFI_HOSTNAME }}

#define DEVICE_PRETTY_NAME {{ cfg.DEVICE_PRETTY_NAME }}

// Access Point configuration.
#define WIFI_CHANNEL {{ cfg.WIFI_CHANNEL }}
#define WIFI_HIDE_SSID {{ cfg.WIFI_HIDE_SSID }}
#define WIFI_MAX_DEVICES {{ cfg.WIFI_MAX_DEVICES }}

// OTA updates.
#define OTA_ENABLE {{ cfg.OTA_ENABLE }}

// You MUST define a password, otherwise OTA won't be enabled.
#define OTA_PASSWORD {{ cfg.OTA_PASSWORD }}

// Server configuration:
#define HTTP_LISTEN_PORT {{ cfg.HTTP_LISTEN_PORT }}
#define HTTP_AUTH_ENABLE {{ cfg.HTTP_AUTH_ENABLE }}
#define HTTP_AUTH_USER {{ cfg.HTTP_AUTH_USER }}
#define HTTP_AUTH_PASS {{ cfg.HTTP_AUTH_PASS }}

// CORS - define allowed origins.
{% if cfg.HTTP_CORS_ALLOW_ORIGIN is defined and cfg.HTTP_CORS_ALLOW_ORIGIN != None %}
#define HTTP_CORS_ALLOW_ORIGIN {{ cfg.HTTP_CORS_ALLOW_ORIGIN }}
{% endif %}

// WebSocket configuration.
#define WS_MAX_CLIENTS {{ cfg.WS_MAX_CLIENTS }}
#define WS_PING_INTERVAL {{ cfg.WS_PING_INTERVAL }}

// Web TTY configuration.
// You can specify any option documented here: https://xtermjs.org/docs/api/terminal/interfaces/iterminaloptions
// Make sure it is a valid JSON and that it's also a valid C string.
#define TTYD_WEB_CONFIG {{ cfg.TTYD_WEB_CONFIG }}

// Don't change.
#if ( BOARD_TYPE == 0 || BOARD_TYPE == 4 )

// LED configuration - only if board type is custom,
// Wi-Se board LEDs are pre-configured in wise_boards.h.
#define LED_WIFI {{ cfg.LED_WIFI }}
#define LED_STATUS {{ cfg.LED_STATUS }}
#define LED_TX {{ cfg.LED_TX }}
#define LED_RX {{ cfg.LED_RX }}

#endif // BOARD_TYPE

// LED timings (milliseconds).
#define LED_ON_TIME {{ cfg.LED_ON_TIME }}
#define LED_OFF_TIME {{ cfg.LED_OFF_TIME }}

// GPIOs that can be used to control target system.
#define TARGET_GPIO_COUNT {{ cfg.TARGET_GPIO_COUNT }}
#define TARGET_GPIO_INITS {{ cfg.TARGET_GPIO_INITS }}
#define TARGET_GPIO_STRINGS {{ cfg.TARGET_GPIO_STRINGS }}
#define TARGET_GPIO_ONINIT 2
#define TARGET_GPIO_LOCKED 3
#define TARGET_GPIO_UNLOCKED 4

// Advanced buffering parameters:
// Tweak if you feel brave. Report any improvements, but make sure you test them at 1500000 8N1 and that it works better
// than the defaults before submitting.
// Note that these buffers do not cause measurable latency, they need to be sort of high so that the WebSocket sender
// can catch up as the UART is being stuffed with high speed data.

#define UART_RX_BUF_SIZE {{ cfg.UART_RX_BUF_SIZE }}
#define UART_RX_SOFT_MIN {{ cfg.UART_RX_SOFT_MIN }}
#define UART_BUFFER_BELOW_SOFT_MIN_DYNAMIC_DELAY {{ cfg.UART_BUFFER_BELOW_SOFT_MIN_DYNAMIC_DELAY }}

// Automatic baudrate detection interval.
#define UART_AUTOBAUD_TIMEOUT_MILLIS {{ cfg.UART_AUTOBAUD_TIMEOUT_MILLIS }}
#define UART_AUTOBAUD_ATTEMPT_INTERVAL {{ cfg.UART_AUTOBAUD_ATTEMPT_INTERVAL }}

// UART software flow control, improves stability.
// It must be supported by the connected device for it to make any difference.
#define UART_SW_FLOW_CONTROL {{ cfg.UART_SW_FLOW_CONTROL }}
#define UART_SW_FLOW_CONTROL_LOW_WATERMARK {{ cfg.UART_SW_FLOW_CONTROL_LOW_WATERMARK }}
#define UART_SW_FLOW_CONTROL_HIGH_WATERMARK {{ cfg.UART_SW_FLOW_CONTROL_HIGH_WATERMARK }}
#define UART_SW_LOCAL_FLOW_CONTROL_STOP_MAX_MS {{ cfg.UART_SW_LOCAL_FLOW_CONTROL_STOP_MAX_MS }}

#define WS_SEND_BUF_SIZE {{ cfg.WS_SEND_BUF_SIZE }}

#define HEAP_FREE_LOW_WATERMARK {{ cfg.HEAP_FREE_LOW_WATERMARK }}
#define HEAP_FREE_HIGH_WATERMARK {{ cfg.HEAP_FREE_HIGH_WATERMARK }}

// If we stopped for half a second and the heap is still stuffed like a turkey we might just as well crash instead of
// continue waiting, this code is probably leaky AF anyway.
#define HEAP_CAUSED_WS_FLOW_CTL_STOP_MAX_MS {{ cfg.HEAP_CAUSED_WS_FLOW_CTL_STOP_MAX_MS }}

// End of configuration.
#include "wise_boards.h"

#endif // WI_SE_SW_CONFIG_H
