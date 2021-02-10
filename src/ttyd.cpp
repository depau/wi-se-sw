//
// Created by depau on 1/27/21.
//

#include <ArduinoJson.h>
#include <cstdio>
#include <debug.h>

#include "config.h"
#include "server.h"
#include "ttyd.h"
#include "ws_nocopy.h"

void TTY::stty(uint32_t baudrate, uint8_t config) {
    this->uartBaudRate = baudrate;
    this->uartConfig = config;

    debugf("TTY stty baud %d config %02X\r\n", baudrate, config);

    if (uartBegun) {
        UART_COMM.end();
    }

    UART_COMM.setRxBufferSize(UART_RX_BUF_SIZE);
    UART_COMM.begin(baudrate, (SerialConfig) config);
    UART_COMM.setTimeout(1);
    uartBegun = true;

    if (wsClientsLen > 0) {
        sendWindowTitle();
    }
}

void TTY::markClientAuthenticated(uint32_t clientId) {
    wsClients[wsClientsLen++] = clientId;
}

size_t TTY::snprintWindowTitle(char *dest, size_t len) const {
    char bits = '!';
    char parity = '!';
    char stop[3] = "!";

    switch (uartConfig & MASK_UART_BITS) {
        case UART_NB_BIT_5:
            bits = '5';
            break;
        case UART_NB_BIT_6:
            bits = '6';
            break;
        case UART_NB_BIT_7:
            bits = '7';
            break;
        case UART_NB_BIT_8:
            bits = '8';
            break;
    }

    switch (uartConfig & MASK_UART_PARITY) {
        case UART_PARITY_NONE:
            parity = 'N';
            break;
        case UART_PARITY_EVEN:
            parity = 'E';
            break;
        case UART_PARITY_ODD:
            parity = 'O';
    }

    switch (uartConfig & MASK_UART_STOP) {
        case UART_NB_STOP_BIT_0:
            stop[0] = '0';
            break;
        case UART_NB_STOP_BIT_1:
            stop[0] = '1';
            break;
        case UART_NB_STOP_BIT_15:
            strcpy(stop, "15");
            break;
        case UART_NB_STOP_BIT_2:
            stop[0] = '2';
            break;
    }

    return snprintf(dest, len, "%dbps %c%c%s (%s) - Wi-Se", uartBaudRate, bits, parity, stop, WIFI_HOSTNAME);
}

void TTY::sendClientConfiguration(uint32_t clientId) {
    websocket->binary(clientId, ttydWebConfig);
}

void TTY::sendWindowTitle(uint32_t clientId) {
    char windowTitle[100] = {0};
    windowTitle[0] = CMD_SET_WINDOW_TITLE;
    size_t titleLen = 1 + snprintWindowTitle(windowTitle + 1, 99);

    if (clientId < 0) {
        broadcastBufferToClients(reinterpret_cast<uint8_t *>(windowTitle), titleLen);
    } else {
        websocket->binary(clientId, windowTitle);
    }
}

void TTY::sendInitialMessages(uint32_t clientId) {
    debugf("TTY send initial message to %d\r\n", clientId);
    sendWindowTitle(clientId);
    sendClientConfiguration(clientId);
}

bool TTY::isClientAuthenticated(uint32_t clientId) {
    return findClientIndex(clientId) >= 0;
}

void TTY::removeClient(uint32_t clientId) {
    debugf("TTY remove client %d\r\n", clientId);
    bool found = false;

    for (int i = 0; i < wsClientsLen; i++) {
        if (wsClients[i] == clientId) {
            found = true;
        }
        if (found && i < wsClientsLen - 1) {
            wsClients[i] = wsClients[i + 1];
            wsClientsLastSeen[i] = wsClientsLastSeen[i + 1];
        }
    }

    if (found) {
        wsClientsLen--;
    }
}

bool TTY::canHandleClient(uint32_t clientId) const {
    if (wsClientsLen >= WS_MAX_CLIENTS) {
        debugf("TTY too many clients (%d), refusing %d\r\n", wsClientsLen, clientId);
        // Won't accept more clients
        return false;
    }
    return true;
}

void TTY::nukeClient(uint32_t clientId, uint16_t closeReason) {
    debugf("TTY nuke client %d\r\n", clientId);
    this->removeClient(clientId);
    websocket->close(clientId, closeReason);
}

void TTY::handleWebSocketMessage(uint32_t clientId, const uint8_t *buf, size_t len) {
    char command = buf[0];
    bool isAuthToken = false;
    const char *authToken = nullptr;

    debugf("TTY new message, client %d, command %c\r\n", clientId, command);

    if (command == CMD_JSON_DATA) {
        DynamicJsonDocument doc(200);
        deserializeJson(doc, buf, len);

        if (doc.isNull()) {
            debugf("TTY client sent bad auth json %d\r\n", clientId);
            nukeClient(clientId, WS_CLOSE_BAD_DATA);
            return;
        }
        if (doc.containsKey("AuthToken")) {
            isAuthToken = true;
            authToken = doc["AuthToken"];
        }
    }

    if (!isClientAuthenticated(clientId)) {
        if (HTTP_AUTH_ENABLE &&
            (!isAuthToken || authToken == nullptr || strncmp((char *) authToken, token, HTTP_AUTH_TOKEN_LEN) != 0)) {
            debugf("TTY client policy violation %d\r\n", clientId);
            nukeClient(clientId, WS_CLOSE_POLICY_VIOLATION);
            return;
        }
        debugf("TTY client authenticated %d\r\n", clientId);
        markClientAuthenticated(clientId);
        sendInitialMessages(clientId);
    }

    clientSeen(clientId);

    if (command == CMD_INPUT) {
        UART_COMM.write((const char *) buf + 1, len - 1);
        requestLedBlink.leds.tx = true;
    } else if (command == CMD_PAUSE) {
        flowControlRequestStop(FLOW_CTL_SRC_REMOTE);
    } else if (command == CMD_RESUME) {
        flowControlRequestResume(FLOW_CTL_SRC_REMOTE);
    }
    // Resize isn't implemented since... well... people in the 80's didn't expect they'd be resizing terminals 10 years
    // later
}

void TTY::handleWebSocketPong(uint32_t clientId) {
    debugf("TTY client seen %d\r\n", clientId);
    clientSeen(clientId);
}

void TTY::broadcastBufferToClients(uint8_t *buf, size_t len) {
    for (int i = 0; i < wsClientsLen; i++) {
        uint32_t clientId = wsClients[i];
        websocket->binary(clientId, buf, len);
    }
}

void TTY::checkClientTimeouts() {
    uint64_t now = millis();
    for (int i = 0; i < wsClientsLen; i++) {
        uint32_t clientId = wsClients[i];
        uint64_t lastSeen = wsClientsLastSeen[i];

        if (lastSeen + CLIENT_TIMEOUT_MILLIS < now) {
            nukeClient(clientId, WS_CLOSE_OK);
        }
    }
    websocket->cleanupClients(WS_MAX_CLIENTS);
}

void TTY::flowControlRequestStop(uint8_t source) {
    if (!UART_SW_FLOW_CONTROL) {
        return;
    }
    if (flowControlStatus == 0) {
        UART_COMM.write(FLOW_CTL_STOP_CHAR);
    }
    flowControlStatus |= source;
}

void TTY::flowControlRequestResume(uint8_t source) {
    if (!UART_SW_FLOW_CONTROL) {
        return;
    }
    if (flowControlStatus == 0) {
        return;
    }
    flowControlStatus &= ~source;
    if (flowControlStatus == 0) {
        UART_COMM.write(FLOW_CTL_CONT_CHAR);
    }
}

void TTY::pingClients() {
    for (int i = 0; i < wsClientsLen; i++) {
        websocket->ping(wsClients[i]);
    }
}

void TTY::handleLedBlinkRequests() {
    uint64_t now = millis();
    const uint8_t leds[] = {LED_RX, LED_TX};

    for (int i = 0; i < 2; i++) {
        if (ledsBusyUntilMillis.array[i] >= now) {
            if (scheduledLedsOffMillis.array[i] < now) {
                digitalWrite(leds[i], LOW);
            }
        } else {
            if (requestLedBlink.array[i]) {
                requestLedBlink.array[i] = false;
                scheduledLedsOffMillis.array[i] = now + LED_ON_TIME;
                ledsBusyUntilMillis.array[i] = now + LED_ON_TIME + LED_OFF_TIME;
                digitalWrite(leds[i], HIGH);
            }
        }
    }
}

void TTY::performHousekeeping() {
    uint64_t now = millis();
    if (lastLedHandleMillis + LED_HANDLE_EVERY_MILLIS < now) {
        lastLedHandleMillis = now;
        //debugf("TTY handle LED\r\n");
        handleLedBlinkRequests();
    }
    if (lastClientTimeoutCheckMillis + CLIENT_TIMEOUT_CHECK_EVERY_MILLIS < now) {
        lastClientTimeoutCheckMillis = now;
        debugf("TTY handle timeouts\r\n");
        checkClientTimeouts();
    }
    if (lastClientPingMillis + CLIENT_PING_EVERY_MILLIS < now) {
        lastClientPingMillis = now;
        debugf("TTY handle ping\r\n");
        pingClients();
    }
}

void TTY::dispatchUart() {
    if (wsClientsLen == 0) {
        return;
    }
    size_t available = UART_COMM.available();
    if (!available) {
        return;
    }
    // Rather wait a little bit longer instead of sending a crapload of tiny chunks that take
    if (available < UART_RX_SOFT_MIN) {
        // Wait for roughly the amount of time it takes for an amount of data 2/3 the size of the WS buffer to be
        // received over UART at the current rate, but not for too long so we don't affect responsiveness
        delay(UART_BUFFER_BELOW_SOFT_MIN_DYNAMIC_DELAY);
        available = UART_COMM.available();
    }

    // Request flow stop/continue when the buffer is about to overflow
    bool wsCanSend = wsClientsLen > 0 && websocket->client(wsClients[0])->canSend();
    if (available > UART_SW_FLOW_CONTROL_HIGH_WATERMARK || !wsCanSend) {
        flowControlRequestStop(FLOW_CTL_SRC_LOCAL);
    } else if (available < UART_SW_FLOW_CONTROL_LOW_WATERMARK) {
        flowControlRequestResume(FLOW_CTL_SRC_LOCAL);
    }

    // Don't bother sending if the WebSocket message queue is full, it will be dropped anyway and garble the terminal.
    // We'll come back here at the next iteration.
    if (!wsCanSend) {
        return;
    }

    size_t bufsize = std::min(available + 512, (size_t) WS_SEND_BUF_SIZE);

    // Buffer is placed in heap so that if all goes well we can only perform one copy and speed the process up
    // significantly. We can free it later if the slow path is the only option.
    //
    // +1 for ttyd command, +1 for the null terminator AsyncWebSocket wants
    char *buf = (char *) malloc((bufsize + 2) * sizeof(char));

    buf[0] = CMD_OUTPUT;

    // Read directly into the buffer
    uint8_t t1;
    BENCH t1 = micros64();

    size_t read = UART_COMM.readBytes(buf + 1, bufsize);

    //BENCH UART_DEBUG.printf("READ %dB time %lld\n", read, micros64() - t1);

    if (read == 0) {
        free(buf);
        return;
    }

    read++;  // Take the command into account
    buf = (char *) realloc(buf, (read + 1) * sizeof(char));  // Also take terminator into account

    requestLedBlink.leds.rx = true;

    BENCH t1 = micros64();

    // Use the no-copy websocket message to send to one single client
    // Read first to avoid race conditions, then check if there's actually only one client
    uint32_t singleClientId = wsClients[0];
    if (wsClientsLen == 1) {
        // Fast path is usable! The buffer will not be copied any further.
        debugf("Sending through FAST path\r\n");
        AsyncWebSocketMessage *message = new AsyncWebSocketBasicMessageNoCopy((const char *) buf, read + 1, WS_BINARY);
        websocket->message(singleClientId, message);
        // The buffer is not being freed on purpose since AsyncWebSocket will do it later
    } else {
        // We need to go the slow path - the buffer will end up being copied wsClientLen + 1 times
        debugf("Sending through SLOW path\r\n");
        broadcastBufferToClients((uint8_t *) buf, read + 1);
        free(buf);
    }

    //BENCH UART_DEBUG.printf("WSEND %dB time %lld\n", read, micros64() - t1);
};