//
// Created by depau on 3/13/21.
//

#include <Arduino.h>
#include <uart.h>
#ifdef ESP8266
    #include <esp8266_peri.h>
    #include <user_interface.h>
#else
    #include "driver/uart.h"
#endif
#include "ExtendedSerial.h"

int ExtendedSerial::autobaudMeasure() {
    static bool doTrigger = true;

    if (doTrigger) {
#ifdef ESP8266
        uart_start_detect_baudrate(_uart_nr);
#else
        uartStartDetectBaudrate(_uart);
#endif
        doTrigger = false;
    }

#ifdef ESP8266
    int32_t divisor = uart_baudrate_detect(_uart_nr, 1);
#else
    int32_t divisor = uartDetectBaudrate(_uart);
#endif
    if (!divisor) {
        return 0;
    }

    // Initialize for a next round.
    doTrigger = true;
    int32_t baudrate = UART_CLK_FREQ / divisor;

    return baudrate;
}

int ExtendedSerial::autobaudGetClosestStdRate(int32_t rawBaud) {
    static const int default_rates[] = {300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400,
                                        256000, 460800, 921600, 1500000, 1843200, 3686400};

    size_t i;
    // Find the nearest real baudrate.
    for (i = 1; i < sizeof(default_rates) / sizeof(default_rates[0]) - 1; i++) {
        if (rawBaud <= default_rates[i]) {
            if (rawBaud - default_rates[i - 1] < default_rates[i] - rawBaud) {
                i--;
            }
            break;
        }
    }

    return default_rates[i];
}

void ExtendedSerial::sendBreak() {
#ifdef ESP8266
    uart_wait_tx_empty(_uart);
    USC0(_uart_nr) |= BIT(UCBRK);

    // 10ms should be enough to convince agetty we sent a break without breaking Wi-Fi.
    delayMicroseconds(10 * 1000);
    USC0(_uart_nr) &= ~BIT(UCBRK);
#else
    uart_wait_tx_done(_uart_nr, pdMS_TO_TICKS(10));
    uart_send_break(_uart_nr);
#endif
}

ExtendedSerial ExtSerial0(UART0);
ExtendedSerial ExtSerial1(UART1);
