//
// Created by depau on 1/26/21.
//

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "html.h"
#include "server.h"


void WiSeServer::begin() {
    httpd->addHandler(websocket);
    httpd->on("/", HTTP_GET, handleIndex);
    httpd->on("/index.html", HTTP_GET, handleIndex);
    httpd->on("/token", HTTP_GET,
              std::bind(&WiSeServer::handleToken, this, std::placeholders::_1));
    httpd->on("/stty", HTTP_GET | HTTP_POST,
              std::bind(&WiSeServer::handleSttyRequest, this, std::placeholders::_1),
              nullptr,
              std::bind(&WiSeServer::handleSttyBody, this, std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
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
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, index_html_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}


void WiSeServer::handleToken(AsyncWebServerRequest *request) const {
    if (!checkHttpBasicAuth(request)) return;
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
            doc["parity"] = -1;
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

void sttyBadRequest(AsyncWebServerRequest *request) {
    request->send(400, "text/plain", "Invalid input in JSON");
}

void WiSeServer::handleSttyRequest(AsyncWebServerRequest *request) const {
    if (request->method() == HTTP_GET) {
        sttySendResponse(request);
    }
}

void
WiSeServer::handleSttyBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index,
                           size_t total) const {
    if (!checkHttpBasicAuth(request)) return;

    if (request->method() == HTTP_POST) {
        DynamicJsonDocument doc(200);
        deserializeJson(doc, data, len);

        uint32_t baudrate = ttyd->getUartBaudRate();
        uint8_t uartConfig = ttyd->getUartConfig();

        if (doc.isNull()) {
            return sttyBadRequest(request);
        }

        if (doc.containsKey("baudrate")) {
            baudrate = doc["baudrate"];
        }

        if (doc.containsKey("bits")) {
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
                    return sttyBadRequest(request);
            }
        }

        if (doc.containsKey("parity")) {
            switch ((uint8_t) doc["parity"]) {
                case -1:
                    uartConfig = (uartConfig & ~MASK_UART_PARITY) | UART_PARITY_NONE;
                case 0:
                    uartConfig = (uartConfig & ~MASK_UART_PARITY) | UART_PARITY_EVEN;
                    break;
                case 1:
                    uartConfig = (uartConfig & ~MASK_UART_PARITY) | UART_PARITY_ODD;
                    break;
                default:
                    return sttyBadRequest(request);
            }
        }

        if (doc.containsKey("stop")) {
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
                    return sttyBadRequest(request);
            }
        }

        ttyd->stty(baudrate, uartConfig);
        sttySendResponse(request);
    }
}
