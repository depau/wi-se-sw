//
// Created by depau on 3/6/21.
//

#ifndef WI_SE_SW_SERIAL_H
#define WI_SE_SW_SERIAL_H

#include "Stream.h"
#include "Arduino.h"
#include "ArduinoTime.h"
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <algorithm>
#include "uart.h"
#include "Serial.h"

#define FAKESERIAL_BUF_LEN 10000

enum SerialConfig {
    SERIAL_5N1 = UART_5N1,
    SERIAL_6N1 = UART_6N1,
    SERIAL_7N1 = UART_7N1,
    SERIAL_8N1 = UART_8N1,
    SERIAL_5N2 = UART_5N2,
    SERIAL_6N2 = UART_6N2,
    SERIAL_7N2 = UART_7N2,
    SERIAL_8N2 = UART_8N2,
    SERIAL_5E1 = UART_5E1,
    SERIAL_6E1 = UART_6E1,
    SERIAL_7E1 = UART_7E1,
    SERIAL_8E1 = UART_8E1,
    SERIAL_5E2 = UART_5E2,
    SERIAL_6E2 = UART_6E2,
    SERIAL_7E2 = UART_7E2,
    SERIAL_8E2 = UART_8E2,
    SERIAL_5O1 = UART_5O1,
    SERIAL_6O1 = UART_6O1,
    SERIAL_7O1 = UART_7O1,
    SERIAL_8O1 = UART_8O1,
    SERIAL_5O2 = UART_5O2,
    SERIAL_6O2 = UART_6O2,
    SERIAL_7O2 = UART_7O2,
    SERIAL_8O2 = UART_8O2,
};

enum SerialMode {
    SERIAL_FULL = UART_FULL,
    SERIAL_RX_ONLY = UART_RX_ONLY,
    SERIAL_TX_ONLY = UART_TX_ONLY
};


class FakeSerial : public Stream {
private:
    FILE *fd;
    char buffer[FAKESERIAL_BUF_LEN] = {0};
    size_t seekPos = 0;
    size_t bufLen = 0;
    double rate = 115200 / 8;

public:
    FakeSerial(FILE *fd) : fd{fd} {}

    virtual ~FakeSerial() = default;;

    void begin(unsigned long baud) {
        begin(baud, SERIAL_8N1, SERIAL_FULL, 1, false);
    }

    void begin(unsigned long baud, SerialConfig config) {
        begin(baud, config, SERIAL_FULL, 1, false);
    }

    void begin(unsigned long baud, SerialConfig config, SerialMode mode) {
        begin(baud, config, mode, 1, false);
    }

    void begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin) {
        begin(baud, config, mode, tx_pin, false);
    }

    void begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin, bool invert) {
        rate = ((double) baud) / 8.0;
    }

    void end() {}

    void simulateBaudrate(uint64_t callTimeUs, size_t bytesTransceived) {
#ifdef SIMULATE_BAUDRATE
        uint64_t now = micros();
        uint64_t transferDuration = (uint64_t) (bytesTransceived * 1000000 / rate);
        delayMicrosecondsNoYield((callTimeUs - now) + transferDuration);
#endif
    }

    uint32_t detectBaudrate(time_t timeoutMillis) {
        return 115200;
    }

    size_t write(uint8_t uint8) override {
        uint64_t now = micros();
        fputc(uint8, fd);
        simulateBaudrate(now, 1);
        return 1;
    }

    size_t write(const uint8_t *outBuffer, size_t size) override {
        uint64_t now = micros();
        size_t ret = fwrite(outBuffer, sizeof(uint8_t), size, fd);
        fflush(fd);
        simulateBaudrate(now, size);
        return ret;
    }

    void flush() override {
        fflush(fd);
    }

    int available() override {
//        fd_set readfds;
//        FD_ZERO(&readfds);
//        FD_SET(STDIN_FILENO, &readfds);
//        struct timeval timeout{};
//        timeout.tv_sec = 0;
//        timeout.tv_usec = 0;
//        if (select(1, &readfds, nullptr, nullptr, &timeout) <= 0) {
//            return 0;
//        }
        size_t bread = ::read(fileno(stdin), buffer + seekPos, FAKESERIAL_BUF_LEN - bufLen);
        if (bread != -1 || errno != EWOULDBLOCK) {
            bufLen += bread;  // NOLINT(cppcoreguidelines-narrowing-conversions)
        }
        return bufLen - seekPos;
    }

    int read() override {
        simulateBaudrate(0, 1);
        if (bufLen == 0) {
            return fgetc(stdin);
        }
        return buffer[seekPos++];
    }

    int peek() override {
        if (bufLen == 0) {
            int c = fgetc(stdin);
            buffer[0] = c; // NOLINT(cppcoreguidelines-narrowing-conversions)
            return c;
        }
        return buffer[seekPos];
    }

    size_t readBytes(char *outBuffer, size_t length) override {
        uint64_t now = micros();
        if (bufLen == 0) {
            available();
        }
        if (bufLen == 0) {
            return 0;
        }
        size_t readLen = std::min(length, bufLen - seekPos);
        // Copy internal buffer into output buffer
        memcpy(outBuffer, buffer + seekPos, sizeof(char) * readLen);
        seekPos += readLen;
        if (seekPos >= bufLen) {
            // Reset internal buffer
            seekPos = 0;
            bufLen = 0;
        } else if (seekPos < bufLen) {
            // Move internal buffer contents to the beginning
            memcpy(buffer, buffer + seekPos, sizeof(char) * (bufLen - seekPos));
            bufLen -= seekPos;
            seekPos = 0;
        }
        simulateBaudrate(now, readLen);
        return readLen;
    }

    size_t readBytes(uint8_t *outBuffer, size_t length) override {
        return readBytes((char *) buffer, length);
    }

    String readString() override {
        char buf[1000];
        size_t len = readBytes(buf, 999);
        buf[len] = 0;
        return String(buf);
    }

    size_t setRxBufferSize(size_t size) { return size; }

    bool operator!=(const FakeSerial &other) const {
        return fd == other.fd;
    }
};

extern FakeSerial Serial;
extern FakeSerial Serial1;

#endif //WI_SE_SW_SERIAL_H
