#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <math.h>

#include "config.h"
#include "server.h"
#include "debug.h"

char token[HTTP_AUTH_TOKEN_LEN + 1] = {0};
AsyncWebServer httpd = AsyncWebServer(HTTP_LISTEN_PORT);
AsyncWebSocket websocket = AsyncWebSocket("/ws");

TTY ttyd(token, &websocket);
WiSeServer server(token, &httpd, &websocket, &ttyd);

bool otaRunning = false;

void blinkError() {
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_STATUS, LOW);
    digitalWrite(LED_RX, LOW);
    digitalWrite(LED_TX, LOW);

    for (int i = 0; i <= 5; i++) {
        digitalWrite(LED_RX, i % 2 == 0);
        digitalWrite(LED_TX, i % 2 != 0);
        delay(200);
    }
}

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

    debugf("\r\n\r\n");
    if (WIFI_MODE == WIFI_STA) {
        debugf("Wi-Fi STA connecting\r\n");

        WiFi.setOutputPower(17.5);
        WiFi.setPhyMode(WIFI_PHY_MODE_11N);
        WiFi.setSleepMode(WIFI_NONE_SLEEP);

        analogWriteRange(0xFF);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        while (WiFi.status() != WL_CONNECTED) {
            for (int i = 0; i < 0xFF; i++) {
                analogWrite(LED_WIFI, i);
                delay(1);
            }
            for (int i = 0xFF; i >= 0; i--) {
                analogWrite(LED_WIFI, i);
                delay(1);
            }
        }

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

#if OTA_ENABLE == 1
    if (OTA_PASSWORD[0] != '\0') {
        ArduinoOTA.setPassword(OTA_PASSWORD);
        ArduinoOTA.setRebootOnSuccess(true);

        ArduinoOTA.setHostname(WIFI_HOSTNAME);

        ArduinoOTA.onStart([]() {
            debugf("Shutting down for OTA");
            otaRunning = true;

            ttyd.shrinkBuffers();
            server.end();
            httpd.end();

            // Blink all LEDs
            uint8_t leds[LED_COUNT] = LED_ORDER;
            for (int i = 0; i <= 5; i++) {
                uint8_t ledOn = i % 2 != 0;
                for (uint8_t led : leds) {
                    digitalWrite(led, ledOn);
                }
                delay(200);
            }
        });

        ArduinoOTA.onProgress([](uint32_t progress, uint32_t total) {
            // Use LEDs as progress bar
            uint8_t leds[LED_COUNT] = LED_ORDER;
            uint32_t ledThresh = total / LED_COUNT;
            uint8_t curLed = _min(LED_COUNT, progress / ledThresh);
            uint32_t ledProgress = map(progress % LED_COUNT, 0, ledThresh, 0, 255);
            analogWrite(leds[curLed], ledProgress);
        });

        ArduinoOTA.onEnd([]() {
            // Animate all LEDs towards the blue one
            uint8_t leds[LED_COUNT] = LED_ORDER;

            for (int led = LED_COUNT - 1; led > 1; led--) {
                for (int i = 255; i >= 0; i--) {
                    analogWrite(led, i);
                    delay(1);
                }
            }
        });

        ArduinoOTA.onError([](ota_error_t _) {
            // Blink red LEDs, then reset
            blinkError();
            ESP.reset();
        });

        ArduinoOTA.begin(false);
    }
#endif
}

void loop() {
    if (otaRunning) {
        return yield();
    }

    ttyd.dispatchUart();
    yield();
    ttyd.performHousekeeping();
    yield();
}