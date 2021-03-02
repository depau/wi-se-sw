//
// Created by depau on 1/26/21.
//

#ifndef WI_SE_SW_WISE_BOARDS_H
#define WI_SE_SW_WISE_BOARDS_H

#define BOARD_CUSTOM 0
#define BOARD_WI_SE_RPI_V01 1
#define BOARD_WI_SE_OPI4_V01 2
#define BOARD_WI_SE_REWIRABLE_V01 3

#if BOARD_TYPE == BOARD_WI_SE_REWIRABLE_V01
#define BOARD_NAME "Wi-Se Rewirable v0.1"
#define LED_WIFI 5
#define LED_STATUS 13
#define LED_TX 14
#define LED_RX 12
#elif BOARD_TYPE == BOARD_WI_SE_RPI_V01 || BOARD_TYPE == BOARD_WI_SE_OPI4_V01
#define LED_WIFI 5
#define LED_STATUS 14
#define LED_TX 13
#define LED_RX 12
#endif //BOARD_TYPE

#if BOARD_TYPE == BOARD_CUSTOM
#define BOARD_NAME "Generic ESP8266 board"
#elif BOARD_TYPE == BOARD_WI_SE_RPI_V01
#define BOARD_NAME "Wi-Se Raspberry Pi Header v0.1"
#elif BOARD_TYPE == BBOARD_WI_SE_OPI4_V01
#define BOARD_NAME "Wi-Se Orange Pi 4 v0.1"
#else
#define BOARD_NAME "Unknown"
#endif

#endif //WI_SE_SW_WISE_BOARDS_H
