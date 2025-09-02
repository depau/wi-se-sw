//
// Created by depau on 1/27/21.
//

#include <ArduinoJson.h>
#include <cstdio>
#include <debug.h>

#include "config.h"
#include "server.h"
#include "ttyd.h"
#include "xschedule.h"
#include "ExtendedSerial.h"

void TTY::begin() {
#if UART_COMM_TX_EN >= 0
    // Enable TX line to allow transmission.
    pinMode(UART_COMM_TX_EN, OUTPUT);
    digitalWrite(UART_COMM_TX_EN, LOW);
#endif
#if TARGET_GPIO_COUNT > 0
    // Configure target gpios.
    for (size_t i = 0; i < TARGET_GPIO_COUNT; i++) {
        debugf("Configure gpio %u from index %u to mode %u.\r\n", gpioConfigs[i].gpio, i, gpioConfigs[i].mode);
        pinMode(gpioConfigs[i].gpio, gpioConfigs[i].mode);

        // When requested set gpio in init phase.
        if (gpioConfigs[i].lock == TARGET_GPIO_ONINIT) {
            gpioConfigs[i].lock = TARGET_GPIO_UNLOCKED;
            gpioConfigs[i].state = *gpioConfigs[i].dval;

            debugf("Target gpio %u from index %u unlocked during init.\r\n", gpioConfigs[i].gpio, i);
            for (size_t x = 0; x < TARGET_GPIO_COUNT; x++) {
                if ((x != i) && (gpioConfigs[x].gpio == gpioConfigs[i].gpio)) {
                    debugf("Target gpio %u from index %u locked by the same gpio from index %u during init.\r\n", gpioConfigs[x].gpio, x, i);
                    gpioConfigs[x].lock = TARGET_GPIO_LOCKED;
                }
            }
        }
    }
#endif
}

void TTY::end() {
    UART_COMM.flush();
    UART_COMM.end();
    UART_COMM.setRxBufferSize(256);
#if UART_COMM_TX_EN >= 0
    // Disable TX line to prevent debug message on boot.
    digitalWrite(UART_COMM_TX_EN, HIGH);
#endif
}

void TTY::stty(uint32_t baudrate, uint8_t config) {
    this->uartBaudRate = baudrate;
    this->uartConfig = config;

    debugf("TTY stty baud %d config %02X\r\n", baudrate, config);

    if (uartBegun) {
        UART_COMM.flush();
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
    pendingAuthClients--;
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

    return snprintf(dest, len, "%dbps %c%c%s (%s) - Wi-Se", uartBaudRate, bits, parity, stop, DEVICE_PRETTY_NAME);
}

void TTY::sendClientConfiguration(uint32_t clientId) {
    websocket->binary(clientId, ttydWebConfig);
}

void TTY::sendWindowTitle(int64_t clientId) {
    char windowTitle[100] = {0};
    windowTitle[0] = CMD_SET_WINDOW_TITLE;
    size_t titleLen = 1 + snprintWindowTitle(windowTitle + 1, 99);
    AsyncWebSocketMessageBuffer *wsBuffer = websocket->makeBuffer((uint8_t *) windowTitle, titleLen);
    if (!wsBuffer) return;
    if (clientId < 0) {
        broadcastBufferToClients(wsBuffer);
    } else {
        websocket->client(clientId)->binary(wsBuffer);
    }
}

void TTY::sendInitialMessages(uint32_t clientId) {
    debugf("TTY send initial message to %d\r\n", clientId);
    // Using schedule_function seams to fix all esp crashes
    // during intensive browser page refresh.
    schedule_function([this, clientId]() {
        sendWindowTitle(clientId);
    });

    schedule_function([this, clientId]() {
        sendClientConfiguration(clientId);
    });

#if TARGET_GPIO_COUNT > 0
    // Force sending current GPIOs states
    // in case of new client or reconnections.
    schedule_function([this]() {
        sendGpioStates(CMD_SERVER_GPIO_STATES);
    });
#endif
}

bool TTY::isClientAuthenticated(uint32_t clientId) {
    return findClientIndex(clientId) >= 0;
}

void TTY::removeClient(uint32_t clientId) {
    debugf("TTY remove client %d\r\n", clientId);
    bool found = false;

    blockClient(clientId);

    for (int i = 0; i < wsClientsLen; i++) {
        if (wsClients[i] == clientId) {
            found = true;
            wsClients[i] = -1;
        }

        if (found && i < wsClientsLen - 1 && i < WS_MAX_CLIENTS - 1) {
            wsClients[i] = wsClients[i + 1];
            wsClientsLastSeen[i] = wsClientsLastSeen[i + 1];
        }
    }

    if (found) {
        wsClientsLen--;
    }
}

void TTY::blockClient(uint32_t clientId) {
    debugf("TTY client blocked: %d\r\n", clientId);
    wsBlockedClients[wsBlockedClientsLen++] = clientId;
    wsClientBlockedAtMillis[wsBlockedClientsLen - 1] = millis();
}

void TTY::removeExpiredClientBlocks() {
    uint64_t now = millis();
    // Iterate backwards and clear out as many expired items from the end.
    for (int i = wsBlockedClientsLen - 1; i >= 0; i--) {
        if (wsClientBlockedAtMillis[i] != 0 && wsClientBlockedAtMillis[i] + WS_CLIENT_BLOCK_EXPIRE_MILLIS < now) {
            debugf("TTY Client unblocked: %d\r\n", wsBlockedClients[i]);
            wsBlockedClients[i] = 0;
            wsClientBlockedAtMillis[i] = 0;
            if (i >= wsBlockedClientsLen - 1) {
                wsBlockedClientsLen--;
            }
        }
    }
    // Iterate forward and fill any holes with items from the end.
    for (int i = 0; i < wsBlockedClientsLen; i++) {
        if (wsClientBlockedAtMillis[i] == 0) {
            wsBlockedClients[i] = wsBlockedClients[wsBlockedClientsLen - 1];
            wsClientBlockedAtMillis[i] = wsClientBlockedAtMillis[wsBlockedClientsLen - 1];
            wsBlockedClients[wsBlockedClientsLen - 1] = 0;
            wsClientBlockedAtMillis[wsBlockedClientsLen - 1] = 0;
            wsBlockedClientsLen--;
        }
        while (wsBlockedClientsLen > 0 && wsClientBlockedAtMillis[wsBlockedClientsLen - 1] == 0) {
            wsBlockedClientsLen--;
        }
    }
}

bool TTY::isClientBlocked(uint32_t clientId) {
    removeExpiredClientBlocks();
    for (int i = 0; i < wsBlockedClientsLen; i++) {
        if (wsBlockedClients[i] == clientId) {
            return true;
        }
    }
    return false;
}

// Returns false if client cannot be handled.
bool TTY::onNewWebSocketClient(uint32_t clientId) {
    if (wsClientsLen >= WS_MAX_CLIENTS) {
        debugf("TTY too many clients (%d), refusing %d\r\n", wsClientsLen, clientId);
        // Won't accept more clients
        return false;
    }
    pendingAuthClients++;
    return true;
}

void TTY::nukeClient(uint32_t clientId, uint16_t closeReason) {
    debugf("TTY nuke client %d\r\n", clientId);
    this->removeClient(clientId);
    websocket->close(clientId, closeReason);
}

void TTY::handleWebSocketMessage(uint32_t clientId, const uint8_t *buf, size_t len, char fragmentCachedCommand) {
    char command = buf[0];
    bool isAuthToken = false;
    char authToken[HTTP_AUTH_TOKEN_LEN];

    debugf("TTY new message, client %d, command %c, cached command %c free heap %d\r\n", clientId, command,
           fragmentCachedCommand, ESP.getFreeHeap());

    if (fragmentCachedCommand != 0 && fragmentCachedCommand != CMD_INPUT) {
        // Do not accept fragmented data unless it's terminal data.
        return nukeClient(clientId, WS_CLOSE_BAD_CONDITION);
    }

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
            const char *tmpToken = doc["AuthToken"];
            strncpy(authToken, tmpToken, HTTP_AUTH_TOKEN_LEN);
        }
    }

    if (!isClientAuthenticated(clientId)) {
        if (HTTP_AUTH_ENABLE &&
            (!isAuthToken || authToken[0] == 0 || strncmp((char *) authToken, token, HTTP_AUTH_TOKEN_LEN) != 0)) {
            debugf("TTY client policy violation %d\r\n", clientId);
            nukeClient(clientId, WS_CLOSE_POLICY_VIOLATION);
            return;
        }
        debugf("TTY client authenticated %d\r\n", clientId);
        markClientAuthenticated(clientId);
        sendInitialMessages(clientId);
    }

    clientSeen(clientId);

    const uint8_t *inputDataBuf;
    size_t inputLen;
    if (fragmentCachedCommand == CMD_INPUT) {
        command = fragmentCachedCommand;
        inputDataBuf = buf;
        inputLen = len;
    } else {
        inputDataBuf = buf + 1;
        inputLen = len - 1;
    }

    switch (command) {
        case CMD_INPUT:
            UART_COMM.write((const uint8_t *) inputDataBuf, inputLen);
            totalTx += len - 1;
            requestLedBlink.leds.tx = true;
            break;
        case CMD_DETECT_BAUD:
            debugf("TTY Requesting baudrate detection\r\n");
            requestAutobaud();
            break;
        case CMD_SEND_BREAK:
            debugf("TTY Send break\r\n");
            UART_COMM.sendBreak();
            break;
        case CMD_PAUSE:
            flowControlUartRequestStop(FLOW_CTL_SRC_REMOTE);
            break;
        case CMD_RESUME:
            flowControlUartRequestResume(FLOW_CTL_SRC_REMOTE);
            break;
        case CMD_JSON_DATA:
        case CMD_RESIZE_TERMINAL:
            // Resize isn't implemented since... well... people in the 80's didn't predict we'd be resizing terminals in 2021.
            break;
        default:
            debugf("TTY Ignoring client invalid data, len %zu first char %c\r\n", len, buf[0]);
            // nukeClient(clientId, WS_CLOSE_BAD_DATA);
    }
}

void TTY::handleWebSocketPong(uint32_t clientId) {
    debugf("TTY client seen %d\r\n", clientId);
    clientSeen(clientId);
}

void TTY::broadcastBufferToClients(AsyncWebSocketMessageBuffer *wsBuffer) {
    if (!wsBuffer) return;

    if (areAllClientsAuthenticated()) {
        // Fast no-copy path
        websocket->binaryAll(wsBuffer);
    } else {
#ifdef LEGACY_LIB
        wsBuffer->lock();
#endif
        for (int i = 0; i < wsClientsLen; i++) {
            AsyncWebSocketClient *client = websocket->client(wsClients[i]);
            if (!client) continue;
            if (client->status() == WS_CONNECTED) {
                client->binary(wsBuffer);
            }
        }
#ifdef LEGACY_LIB
        wsBuffer->unlock();
        websocket->_cleanBuffers();
#endif
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

void TTY::flowControlUartRequestStop(uint8_t source) {
    if (!UART_SW_FLOW_CONTROL) {
        return;
    }
    if (uartFlowControlStatus == 0) {
        debugf("TTY uart flow control XOFF source %d\r\n", source);
        UART_COMM.write(FLOW_CTL_XOFF);
        uartFlowControlEngagedMillis = millis();
    }
    uartFlowControlStatus |= source;
}

void TTY::flowControlUartRequestResume(uint8_t source) {
    if (!UART_SW_FLOW_CONTROL) {
        return;
    }
    if (uartFlowControlStatus == 0) {
        return;
    }
    uartFlowControlStatus &= ~source;
    if (uartFlowControlStatus == 0) {
        debugf("TTY uart flow control XON source %d\r\n", source);
        UART_COMM.write(FLOW_CTL_XON);
    }
}

void TTY::flowControlWebSocketRequest(bool stop) {
    if (wsFlowControlStopped == stop) {
        return;
    }
    debugf("TTY ws flow control enabled: %d\r\n", stop);
    wsFlowControlStopped = stop;
    AsyncWebSocketMessageBuffer *buffer = websocket->makeBuffer(1);
    if (!buffer) return;
    buffer->get()[0] = stop ? CMD_SERVER_PAUSE : CMD_SERVER_RESUME;
    broadcastBufferToClients(buffer);
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

    if (wsFlowControlStopped || uartFlowControlStatus != 0) {
        digitalWrite(LED_STATUS, HIGH);
    } else {
        digitalWrite(LED_STATUS, LOW);
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
    if (lastStatsCollectMillis + COLLECT_STATS_EVERY_MILLIS < now) {
        collectStats();
        lastStatsCollectMillis = millis();
    }
#if TARGET_GPIO_COUNT > 0
    sendGpioStates(0);
#endif
}

bool TTY::wsCanSend() {
    if (wsClientsLen == 0) {
        return false;
    }

    for (int i = 0; i < wsClientsLen; i++) {
        AsyncWebSocketClient *client = websocket->client(wsClients[i]);
        if ((!client) || (client->status() != WS_CONNECTED) || (client->queueIsFull())) {
            return false;
        }
    }

    return true;
}

bool TTY::areAllClientsAuthenticated() const {
    return pendingAuthClients == 0;
}

// Trigger flow control (UART side) based on the UART buffer and WebSocket send queue status.
bool TTY::performFlowControl_SlowWiFi(size_t uartAvailable) {
    bool canSend = wsCanSend();
    if (uartAvailable > UART_SW_FLOW_CONTROL_HIGH_WATERMARK || !canSend) {
        debugf("Uart available: %d, watermark %d, can send? %d\r\n", uartAvailable, UART_SW_FLOW_CONTROL_HIGH_WATERMARK,
               canSend);
        flowControlUartRequestStop(FLOW_CTL_SRC_LOCAL);
    } else if (uartAvailable < UART_SW_FLOW_CONTROL_LOW_WATERMARK) {
        flowControlUartRequestResume(FLOW_CTL_SRC_LOCAL);
    }
    // Don't stop dispatching if the client requested it and UART is still sending.
    // We've got 80K of RAM, the browser has more.
    return (uartFlowControlStatus & FLOW_CTL_SRC_LOCAL) == FLOW_CTL_SRC_LOCAL;
}

void TTY::unlockUartFlowControlIfTimedOut() {
    if (uartFlowControlStatus && uartFlowControlEngagedMillis + UART_SW_LOCAL_FLOW_CONTROL_STOP_MAX_MS < millis()) {
        flowControlUartRequestResume(FLOW_CTL_SRC_LOCAL | FLOW_CTL_SRC_REMOTE);
    }
}

// Trigger flow control (WebSocket side) if the heap is too full.
bool TTY::performFlowControl_HeapFull() {
    if (wsFlowControlStopped) {
        if (    // We blocked for too long, resume communication for at least one iteration.
                wsFlowControlEngagedMillis + HEAP_CAUSED_WS_FLOW_CTL_STOP_MAX_MS > millis() ||
                // Heap is now workable.
                ESP.getFreeHeap() >= HEAP_FREE_HIGH_WATERMARK) {
            flowControlWebSocketRequest(false);
        }
    } else {
        if (ESP.getFreeHeap() <= HEAP_FREE_LOW_WATERMARK) {
            flowControlWebSocketRequest(true);
            wsFlowControlEngagedMillis = millis();
        }
    }
    return wsFlowControlStopped;
}

void TTY::collectStats() {
    uint64_t now = millis();
    uint64_t tx = totalTx - prevTx;
    uint64_t rx = totalRx - prevRx;
    txRate = tx * 8 * 1000 / (now - lastStatsCollectMillis);
    rxRate = rx * 8 * 1000 / (now - lastStatsCollectMillis);
    prevTx = totalTx;
    prevRx = totalRx;
}

void TTY::requestAutobaud() {
    pendingAutobaud = true;
}

void TTY::sendBaurateDetectionResult(int64_t bestApprox, int64_t measured) {
    debugf("TTY Detected baudrate: %lld (measured: %lld)\r\n", bestApprox, measured);
    uint8_t buf[30];
    size_t len = snprintf(reinterpret_cast<char *>(buf), sizeof(buf), "%c%lld,%lld", CMD_SERVER_DETECTED_BAUD,
                          bestApprox, measured);
    auto wsBuf = websocket->makeBuffer(buf, len);
    broadcastBufferToClients(wsBuf);
}

void TTY::autobaud() {
    if (autobaudStartedAtMillis == 0) {
        autobaudStartedAtMillis = millis();
    }
    if (autobaudLastAttemptAtMillis + UART_AUTOBAUD_ATTEMPT_INTERVAL > millis()) {
        return;
    }

    autobaudLastAttemptAtMillis = millis();
    int measured = UART_COMM.autobaudMeasure();

    if (measured) {
        int bestApprox = UART_COMM.autobaudGetClosestStdRate(measured);
        pendingAutobaud = false;
        autobaudStartedAtMillis = 0;
        autobaudLastAttemptAtMillis = 0;
        return sendBaurateDetectionResult(bestApprox, measured);
    }

    if (autobaudStartedAtMillis + UART_AUTOBAUD_TIMEOUT_MILLIS < millis()) {
        pendingAutobaud = false;
        autobaudStartedAtMillis = 0;
        autobaudLastAttemptAtMillis = 0;
        return sendBaurateDetectionResult(0, 0);
    }
}

void TTY::dispatchUart() {
    if (wsClientsLen == 0) {
        // Unlock all flow control.
        flowControlUartRequestResume(FLOW_CTL_SRC_LOCAL | FLOW_CTL_SRC_REMOTE);
        // No clients connected, so we just set the flag.
        wsFlowControlStopped = false;
        return;
    }

    if (pendingAutobaud) {
        autobaud();
    }

    size_t available = UART_COMM.available();
    if (!available) {
        unlockUartFlowControlIfTimedOut();
        return;
    }

    // Rather wait a little bit longer instead of sending a crapload of tiny chunks that take.
    if (available < UART_RX_SOFT_MIN) {
        // Wait for roughly the amount of time it takes for an amount of data 2/3 the size of the WS buffer to be
        // received over UART at the current rate, but not for too long so we don't affect responsiveness.
        delay(UART_BUFFER_BELOW_SOFT_MIN_DYNAMIC_DELAY);
        available = UART_COMM.available();
    }

    performFlowControl_SlowWiFi(available);

    // Avoid flow control deadlocks.
    unlockUartFlowControlIfTimedOut();

    bool shouldContinueDispatching = wsCanSend() && !performFlowControl_HeapFull();

    // Don't process if flow control was engaged due to low heap or if the WebSocket library can't handle our input.
    if (!shouldContinueDispatching) {
        return;
    }

    // Use the WebSocket library buffer so we can use the "messageAll" fast path that doesn't incur in additional copies
    // +1 for ttyd command.
    size_t bufsize = available + 1;
    AsyncWebSocketMessageBuffer *wsBuffer = websocket->makeBuffer(bufsize);
    if (!wsBuffer) return;
    char *buf = (char *) wsBuffer->get();
    if (!buf) return;
    buf[0] = CMD_OUTPUT;

    // uint8_t t1;
    // BENCH t1 = micros64();
    BENCH debugf("Sending %d B to %d clients\r\n", bufsize, wsClientsLen);

    // Read directly into the buffer.
    size_t read = UART_COMM.readBytes(buf + 1, bufsize - 1);
    totalRx += read;

    // BENCH UART_DEBUG.printf("READ %dB time %lld\n", read, micros64() - t1);

    if (read == 0) {
        return;
    }

    requestLedBlink.leds.rx = true;

    // BENCH t1 = micros64();

    broadcastBufferToClients(wsBuffer);
    // BENCH UART_DEBUG.printf("WSEND %dB time %lld\n", read, micros64() - t1);
}

#if TARGET_GPIO_COUNT > 0
GpioConfig* TTY::getGpioConfigs() {
    return gpioConfigs;
}

void TTY::sendGpioStates(char force) {
    uint64_t now = millis();
    char buf[TARGET_GPIO_COUNT + 1] = {0}; // {'G','D','E','A','D'};
    buf[0] = force;

    for (size_t i = 0; i < TARGET_GPIO_COUNT; i++) {
        // INPUTs
        if ((gpioConfigs[i].mode != OUTPUT) && (gpioConfigs[i].mode != OUTPUT_OPEN_DRAIN)) {
            bool value = digitalRead(gpioConfigs[i].gpio) ^ gpioConfigs[i].inverted;
            if (gpioConfigs[i].state != value) {
                gpioConfigs[i].state = value;
                buf[0] = CMD_SERVER_GPIO_STATES;
            }
        // OUTPUTs
        } else {
            // When the state is set above 1.
            if (gpioConfigs[i].state > 1) {
                // Add the time after which the port will be reset.
                if (gpioConfigs[i].lock == TARGET_GPIO_UNLOCKED) {
                    debugf("[%llu] Add timestamp for gpio %u from index %u, state %llu, new state %llu.\r\n", now, gpioConfigs[i].gpio, i, gpioConfigs[i].state, gpioConfigs[i].state + now);
                    gpioConfigs[i].state += now;
                // Check if time elapsed and reset gpio state.
                } else if (now >= gpioConfigs[i].state) {
                    debugf("[%llu] Time elapsed for gpio %u from index %u, state %llu, lock %u.\r\n", now, gpioConfigs[i].gpio, i, gpioConfigs[i].state, gpioConfigs[i].lock);
                    gpioConfigs[i].state = 0;
                    buf[0] = CMD_SERVER_GPIO_STATES;
                }
            }

            // Use internal lock to set gpio only on change or request.
            bool value = (!!gpioConfigs[i].state) ^ gpioConfigs[i].inverted;
            if ((gpioConfigs[i].lock != TARGET_GPIO_LOCKED) && (gpioConfigs[i].lock != value)) {
                gpioConfigs[i].lock = value;
                debugf("[%llu] Writting gpio %u from index %u to value %u.\r\n", now, gpioConfigs[i].gpio, i, value);
                digitalWrite(gpioConfigs[i].gpio, value);
            }
        }
        buf[i + 1] = gpioConfigs[i].state ? '1' : '0';
    }
    // Emit changes
    if ((buf[0] == CMD_SERVER_GPIO_STATES) && wsCanSend()) {
        if (AsyncWebSocketMessageBuffer *wsBuffer = websocket->makeBuffer((uint8_t *) buf, TARGET_GPIO_COUNT + 1))
            broadcastBufferToClients(wsBuffer);
    }
}
#endif
