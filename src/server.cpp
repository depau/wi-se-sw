//
// Created by depau on 1/26/21.
//

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "config.h"
#include "server.h"

AsyncWebServer httpd(HTTP_LISTEN_PORT);
AsyncWebSocket websocket("/ws");

void server_start() {
    httpd.addHandler(&websocket);
    httpd.on("/", HTTP_GET, handleIndex);
    httpd.on("/index.html", HTTP_GET, handleIndex);
    httpd.on("/token", HTTP_GET, handleToken);
    httpd.on("/stty", HTTP_GET | HTTP_POST, handleStty);
}

bool checkHttpBasicAuth(AsyncWebServerRequest *request) {
    if (!HTTP_AUTH_ENABLE) {
        return true;
    }
    if (!request->authenticate(HTTP_AUTH_USER, HTTP_AUTH_PASS)) {
        request->requestAuthentication();
        return false;
    }
    return true;
}

void handleIndex(AsyncWebServerRequest *request) {
    if (!checkHttpBasicAuth(request)) return;
}

void handleStty(AsyncWebServerRequest *request) {
    if (!checkHttpBasicAuth(request)) return;

}

void handleToken(AsyncWebServerRequest *request) {
    if (!checkHttpBasicAuth(request)) return;

}
