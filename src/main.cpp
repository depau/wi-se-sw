#include <Arduino.h>
//#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>

#include "config.h"
#include "server.h"
#include "debug.h"

char token[HTTP_AUTH_TOKEN_LEN + 1] = {0};
AsyncWebServer httpd = AsyncWebServer(HTTP_LISTEN_PORT);
AsyncWebSocket websocket = AsyncWebSocket("/ws");

TTY ttyd(token, &websocket);
WiSeServer server(token, &httpd, &websocket, &ttyd);

void setup() {
    // Init UART
    ttyd.stty(UART_COMM_BAUD, UART_COMM_CONFIG);

#if ENABLE_DEBUG == 1
    if (UART_COMM != UART_DEBUG) {
        UART_DEBUG.begin(UART_DEBUG_BAUD, (SerialConfig) UART_DEBUG_CONFIG);
    }
#endif

    // Init LEDs
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_STATUS, OUTPUT);
    pinMode(LED_RX, OUTPUT);
    pinMode(LED_TX, OUTPUT);

    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_RX, LOW);
    digitalWrite(LED_TX, LOW);
    digitalWrite(LED_STATUS, HIGH);

    // Init Wi-Fi
    WiFi.mode(WIFI_MODE);
    WiFi.hostname(WIFI_HOSTNAME);

    if (WIFI_MODE == WIFI_STA) {
        debugf("Wi-Fi STA connecting\r\n");

        // Blink Wi-Fi LED
        analogWriteFreq(2);
        analogWrite(LED_WIFI, 255);

        WiFi.setOutputPower(17.5);
        WiFi.setPhyMode(WIFI_PHY_MODE_11N);
        WiFi.setSleepMode(WIFI_NONE_SLEEP);

        WiFi.begin(WIFI_SSID, WIFI_PASS);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
        }

        analogWrite(LED_WIFI, 0);
        analogWriteFreq(1000);

        debugf("Wi-Fi STA connected\r\n");
    } else {
        debugf("Turning on soft AP\r\n");
        WiFi.softAP(WIFI_SSID, WIFI_PASS, WIFI_CHANNEL, WIFI_HIDE_SSID, WIFI_MAX_DEVICES);
    }

    digitalWrite(LED_WIFI, HIGH);
    digitalWrite(LED_STATUS, LOW);

    // Set-up Arduino OTA
    // TODO: actually do it
    // ArduinoOTA.setHostname(WIFI_HOSTNAME);

    // Generate token
    if (HTTP_AUTH_ENABLE) {
        const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!/?_=;':";
        for (int i; i < HTTP_AUTH_TOKEN_LEN; i++) {
            token[i] = charset[ESP.random() % (int) (sizeof charset - 1)];
        }
    }

    server.begin();
    httpd.begin();
    debugf("HTTP server is up\r\n");
}

void loop() {
    ttyd.dispatchUart();
    yield();
    ttyd.performHousekeeping();
    yield();
}