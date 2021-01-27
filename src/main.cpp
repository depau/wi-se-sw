#include <Arduino.h>
//#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "config.h"
#include "server.h"

char token[17];
AsyncWebServer httpd = AsyncWebServer(HTTP_LISTEN_PORT);
AsyncWebSocket websocket = AsyncWebSocket("/ws");

TTY ttyd(token, &websocket);
WiSeServer server(token, &httpd, &websocket, &ttyd);

void setup() {
    // Init UART
    UART_COMM.setRxBufferSize(4096);
    UART_COMM.begin(UART_COMM_BAUD, (SerialConfig) UART_COMM_CONFIG);
    if (UART_COMM != UART_DEBUG) {
        UART_DEBUG.begin(UART_DEBUG_BAUD, (SerialConfig) UART_DEBUG_CONFIG);
    }

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
        UART_DEBUG.println(F("Wi-Fi STA connecting"));

        // Blink Wi-Fi LED
        analogWriteFreq(2);
        analogWrite(LED_WIFI, 255);

        do {
            UART_DEBUG.println(F("Connection attempt"));
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        } while (WiFi.waitForConnectResult() != WL_CONNECTED);

        analogWrite(LED_WIFI, 0);
        analogWriteFreq(1000);

        UART_DEBUG.println(F("Wi-Fi STA connected"));
    } else {
        UART_DEBUG.println(F("Turning on soft AP"));
        WiFi.softAP(WIFI_SSID, WIFI_PASS, WIFI_CHANNEL, WIFI_HIDE_SSID, WIFI_MAX_DEVICES);
    }

    digitalWrite(LED_WIFI, HIGH);
    digitalWrite(LED_STATUS, LOW);

    // Set-up Arduino OTA
    // TODO: actually do it
    // ArduinoOTA.setHostname(WIFI_HOSTNAME);

    // Set up mDNS
    MDNS.addService("http", "tcp", 80);

    // Generate token
    if (HTTP_AUTH_ENABLE) {
        const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!/?_=;':";
        for (int i; i < sizeof(token) - 1; i++) {
            token[i] = charset[ESP.random() % (int) (sizeof charset - 1)];
        }
    }

    server.start();
    httpd.begin();
}

void loop() {
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_BUILTIN, HIGH);

    // wait for a second
    delay(1000);

    // turn the LED off by making the voltage LOW
    digitalWrite(LED_BUILTIN, LOW);

    // wait for a second
    delay(1000);
}