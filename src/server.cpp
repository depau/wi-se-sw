//
// Created by depau on 1/26/21.
//

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "html.h"
#include "server.h"
#include "debug.h"

void WiSeServer::begin() {
    DefaultHeaders::Instance().addHeader("Server", serverHeader);
    DefaultHeaders::Instance().addHeader("X-Ttyd-Implementation", "Wi-Se/C++");
#ifdef CORS_ALLOW_ORIGIN
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", CORS_ALLOW_ORIGIN);
#endif

    // Handle regular HTTP requests
    httpd->on("/", HTTP_GET, handleIndex);
    httpd->on("/index.html", HTTP_GET, handleIndex);
    httpd->on("/token", HTTP_GET,
              std::bind(&WiSeServer::handleToken, this, std::placeholders::_1));
    httpd->on("/stty", HTTP_GET | HTTP_POST,
              std::bind(&WiSeServer::handleSttyRequest, this, std::placeholders::_1),
              nullptr,
              std::bind(&WiSeServer::handleSttyBody, this, std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    httpd->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    // Handle CORS preflight
    httpd->onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });

    // Handle WebSocket connections
    websocket->onEvent(std::bind(&WiSeServer::onWebSocketEvent, this, std::placeholders::_1, std::placeholders::_2,
                                 std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
                                 std::placeholders::_6));
    httpd->addHandler(websocket);
    debugf("Web app server is up\r\n");
}

bool WiSeServer::checkHttpBasicAuth(AsyncWebServerRequest *request) {
    if (!HTTP_AUTH_ENABLE) {
        return true;
    }
    if (!request->authenticate(HTTP_AUTH_USER, HTTP_AUTH_PASS)) {
        request->requestAuthentication();
        return false;
    }
    return true;
}

void WiSeServer::handleIndex(AsyncWebServerRequest *request) {
    if (!checkHttpBasicAuth(request)) return;
    debugf("GET /\r\n");
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, index_html_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}


void WiSeServer::handleToken(AsyncWebServerRequest *request) const {
    if (!checkHttpBasicAuth(request)) return;
    debugf("GET /token\r\n");
    char response[100] = R"({"token": ")";
    strcat(response, this->token);
    strcat(response, "\"}");
    request->send(200, "application/json;charset=utf-8", response);
}


void WiSeServer::sttySendResponse(AsyncWebServerRequest *request) const {
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    DynamicJsonDocument doc(200);

    doc["baudrate"] = ttyd->getUartBaudRate();

    uint8_t uartConfig = ttyd->getUartConfig();
    switch (uartConfig & MASK_UART_BITS) {
        case UART_NB_BIT_5:
            doc["bits"] = 5;
            break;
        case UART_NB_BIT_6:
            doc["bits"] = 6;
            break;
        case UART_NB_BIT_7:
            doc["bits"] = 7;
            break;
        case UART_NB_BIT_8:
            doc["bits"] = 8;
            break;
    }

    switch (uartConfig & MASK_UART_PARITY) {
        case UART_PARITY_NONE:
            doc["parity"] = nullptr;
            break;
        case UART_PARITY_EVEN:
            doc["parity"] = 0;
            break;
        case UART_PARITY_ODD:
            doc["parity"] = 1;
    }

    switch (uartConfig & MASK_UART_STOP) {
        case UART_NB_STOP_BIT_0:
            doc["stop"] = 0;
            break;
        case UART_NB_STOP_BIT_1:
            doc["stop"] = 1;
            break;
        case UART_NB_STOP_BIT_15:
            doc["stop"] = 15;
            break;
        case UART_NB_STOP_BIT_2:
            doc["stop"] = 2;
            break;
    }

    serializeJson(doc, *response);
    request->send(response);
}

void invalidJsonBadRequest(AsyncWebServerRequest *request, const char *message) {
    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    response->printf("Invalid input in JSON: %s", message);
    response->setCode(400);
    request->send(response);
}

void WiSeServer::handleSttyRequest(AsyncWebServerRequest *request) const {
    if (!checkHttpBasicAuth(request)) return;

    if (request->method() == HTTP_GET) {
        debugf("GET /stty\r\n");
        sttySendResponse(request);
    }
}

void
WiSeServer::handleSttyBody(
        AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) const {
    if (!checkHttpBasicAuth(request)) return;

    if (request->method() == HTTP_POST) {
        debugf("POST /stty\r\n");
        DynamicJsonDocument doc(200);
        deserializeJson(doc, data, len);

        uint32_t baudrate = ttyd->getUartBaudRate();
        uint8_t uartConfig = ttyd->getUartConfig();

        if (doc.isNull()) {
            return invalidJsonBadRequest(request, "JSON is invalid");
        }

        if (doc.containsKey("baudrate")) {
            if (!doc["baudrate"].is<unsigned int>()) {
                return invalidJsonBadRequest(request, "\"baudrate\" must be a positive number");
            }
            baudrate = doc["baudrate"];
        }

        if (doc.containsKey("bits")) {
            if (!doc["bits"].is<unsigned int>()) {
                return invalidJsonBadRequest(request, "\"bits\" must be a positive number, one of 5, 6, 7, 8");
            }
            switch ((uint8_t) doc["bits"]) {
                case 5:
                    uartConfig = (uartConfig & ~MASK_UART_BITS) | UART_NB_BIT_5;
                    break;
                case 6:
                    uartConfig = (uartConfig & ~MASK_UART_BITS) | UART_NB_BIT_6;
                    break;
                case 7:
                    uartConfig = (uartConfig & ~MASK_UART_BITS) | UART_NB_BIT_7;
                    break;
                case 8:
                    uartConfig = (uartConfig & ~MASK_UART_BITS) | UART_NB_BIT_8;
                    break;
                default:
                    return invalidJsonBadRequest(request, "\"bits\" must be a positive number, one of 5, 6, 7, 8");
            }
        }

        if (doc.containsKey("parity")) {
            if (doc["parity"].isNull()) {
                uartConfig = (uartConfig & ~MASK_UART_PARITY) | UART_PARITY_NONE;
            } else {
                if (!doc["parity"].is<unsigned int>()) {
                    return invalidJsonBadRequest(
                            request, "\"parity\" must be a number or null, null (none), 0 (even), 1 (odd)");
                }

                switch ((signed int) doc["parity"]) {
                    case 0:
                        uartConfig = (uartConfig & ~MASK_UART_PARITY) | UART_PARITY_EVEN;
                        break;
                    case 1:
                        uartConfig = (uartConfig & ~MASK_UART_PARITY) | UART_PARITY_ODD;
                        break;
                    default:
                        return invalidJsonBadRequest(
                                request, "\"parity\" must be a number or null, null (none), 0 (even), 1 (odd)");
                }
            }
        }

        if (doc.containsKey("stop")) {
            if (!doc["stop"].is<unsigned int>()) {
                return invalidJsonBadRequest(request, "\"stop\" must be a positive number, one of 0, 1, 15, 2");
            }
            switch ((uint8_t) doc["stop"]) {
                case 0:
                    uartConfig = (uartConfig & ~MASK_UART_STOP) | UART_NB_STOP_BIT_0;
                    break;
                case 1:
                    uartConfig = (uartConfig & ~MASK_UART_STOP) | UART_NB_STOP_BIT_1;
                    break;
                case 15:
                    uartConfig = (uartConfig & ~MASK_UART_STOP) | UART_NB_STOP_BIT_15;
                    break;
                case 2:
                    uartConfig = (uartConfig & ~MASK_UART_STOP) | UART_NB_STOP_BIT_2;
                    break;
                default:
                    return invalidJsonBadRequest(request, "\"stop\" must be a positive number, one of 0, 1, 15, 2");
            }
        }

        ttyd->stty(baudrate, uartConfig);
        sttySendResponse(request);
    }
}

void WiSeServer::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
                                  uint8_t *data, size_t len) {
    AwsFrameInfo *info = nullptr;
    uint8_t *buffer = nullptr;
    size_t *buf_len = nullptr;
    bool shouldDispatchMessage = false;

    switch (type) {
        case WS_EVT_CONNECT:
            debugf("WS new client %d\r\n", client->id());
            if (!ttyd->canHandleClient(client->id())) {
                client->close(WS_CLOSE_TOO_BIG);
                websocket->cleanupClients(WS_MAX_CLIENTS);
            }
            break;
        case WS_EVT_DISCONNECT:
            debugf("WS client disconnected %d\r\n", client->id());
            ttyd->removeClient(client->id());
            websocket->cleanupClients(WS_MAX_CLIENTS);
            debugf("DEALLOC CASE DISCONNECT client ID %d\r\n", client->id());
            deallocClientDataBuffer(client->id());
            break;
        case WS_EVT_ERROR:
            debugf("WS client error [%u] error(%u): %s\r\n", client->id(), *((uint16_t *) arg), (char *) data);
            client->printf(R"({"error": "%u: %s"})", *((uint16_t *) arg), (char *) data);
//            ttyd->removeClient(client->id());
//            client->close(WS_CLOSE_BAD_CONDITION);
//            websocket->cleanupClients(WS_MAX_CLIENTS);
//            deallocClientDataBuffer(client->id());
            break;
        case WS_EVT_PONG:
            debugf("WS client pong %d\r\n", client->id());
            ttyd->handleWebSocketPong(client->id());
            break;
        case WS_EVT_DATA:
            info = (AwsFrameInfo *) arg;
            shouldDispatchMessage = false;

            if (info->final && info->index == 0 && info->len == len) {
                // Entire message in one frame
                debugf("WS client data no-frag final %d, len %d\r\n", client->id(), len);
                buffer = data;
                buf_len = &len;
                shouldDispatchMessage = true;
            } else {
                // Message is split into multiple frames or frame is fragmented
                debugf("WS client data frag %d, index %llu len %d\r\n", client->id(), info->index, len);
                int pos = findDataBufferForClient(client->id());
                buffer = clientDataBuffers[pos];
                buf_len = &clientDataBufLens[pos];

                if (*buf_len + len > WS_FRAGMENTED_DATA_BUFFER_SIZE) {
                    debugf("WS client nuke due to buffer overflow %d\r\n", client->id());
                    ttyd->removeClient(client->id());
                    websocket->close(WS_CLOSE_BAD_CONDITION);
                    websocket->cleanupClients(WS_MAX_CLIENTS);
                    debugf("DEALLOC CASE CLIENT INDUCED BUF OVERFLOW client ID %d\r\n", client->id());
                    deallocClientDataBuffer(client->id());
                    break;
                }

                uint8_t *tmpBuffer = buffer + *buf_len;
                memcpy(tmpBuffer, data, len);
                *buf_len += len;

                if (info->final) {
                    shouldDispatchMessage = true;
                }
            }

            break;
        default:;
    }

    if (shouldDispatchMessage) {
        debugf("WS client data dispatch %d, len %d\r\n", client->id(), *buf_len);
        ttyd->handleWebSocketMessage(client->id(), buffer, *buf_len);
        debugf("DEALLOC CASE DISPATCH client ID %d\r\n", client->id());
        deallocClientDataBuffer(client->id());
    }
}