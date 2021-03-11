#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "config.h"
#include "server.h"
#include "debug.h"

ADC_MODE(ADC_VCC);

char token[HTTP_AUTH_TOKEN_LEN + 1] = {0};

AsyncWebServer *httpd;
AsyncWebSocket *websocket;

TTY *ttyd;
WiSeServer *server;

bool otaRunning = false;

void blinkError() {
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_STATUS, LOW);
    digitalWrite(LED_RX, LOW);
    digitalWrite(LED_TX, LOW);

    for (int i = 0; i <= 10; i++) {
        digitalWrite(LED_RX, i % 2 == 0);
        digitalWrite(LED_TX, i % 2 != 0);
        delay(200);
    }
}

void setup() {
    // Generate token
    if (HTTP_AUTH_ENABLE) {
        const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!/?_=;':";
        for (int i = 0; i < HTTP_AUTH_TOKEN_LEN; i++) {
            token[i] = charset[ESP.random() % (int) (sizeof charset - 1)];
        }
    }

    httpd = new AsyncWebServer(HTTP_LISTEN_PORT);
    websocket = new AsyncWebSocket("/ws");
    ttyd = new TTY(token, websocket);
    server = new WiSeServer(token, httpd, websocket, ttyd);

    // Init UART
    ttyd->stty(UART_COMM_BAUD, UART_COMM_CONFIG);

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

        char wifiSsid[30] = {0};
        if (WIFI_SSID == nullptr) {
            snprintf(wifiSsid, sizeof(wifiSsid), "Wi-Se_%04X", ESP.getChipId() & 0xFFFF);
        } else {
            strncpy(wifiSsid, WIFI_SSID, sizeof(wifiSsid));
        }

        WiFi.softAP(wifiSsid, WIFI_PASS, WIFI_CHANNEL, WIFI_HIDE_SSID, WIFI_MAX_DEVICES);
    }

    //digitalWrite(LED_WIFI, HIGH); // too bright
    analogWrite(LED_WIFI, 40);
    digitalWrite(LED_STATUS, LOW);

    // Set-up Arduino OTA
    // TODO: actually do it
    // ArduinoOTA.setHostname(WIFI_HOSTNAME);


    server->begin();
    httpd->begin();
    debugf("HTTP server is up\r\n");

    MDNS.begin(WIFI_HOSTNAME);

#if OTA_ENABLE == 1
    if (OTA_PASSWORD[0] != '\0') {
        ArduinoOTA.setPassword(OTA_PASSWORD);
        ArduinoOTA.setRebootOnSuccess(true);

        ArduinoOTA.setHostname(WIFI_HOSTNAME);
        ArduinoOTA.setPort(8266);

        ArduinoOTA.onStart([]() {
            otaRunning = true;

            debugf("Shutting down for OTA\r\n");
            UART_DEBUG.flush();

            delay(50);

            ttyd->shrinkBuffers();
            server->end();
            httpd->end();

            // LED animation
            uint8_t leds[LED_COUNT] = LED_ORDER;

            for (int count = 0; count < 2; count++) {
                for (int i = 0; i < 255; i++) {
                    analogWrite(leds[LED_COUNT - 1], i);
                    delayMicroseconds(400);
                }
                for (int led = LED_COUNT - 2; led >= 0; led--) {
                    for (int i = 0; i < 255; i++) {
                        analogWrite(leds[led], i);
                        analogWrite(leds[led + 1], 255 - i);
                        delayMicroseconds(400);
                    }
                    digitalWrite(leds[led], HIGH);
                    digitalWrite(leds[led + 1], LOW);
                }
                for (int i = 255; i >= 0; i--) {
                    analogWrite(leds[0], i);
                    delayMicroseconds(400);
                }
            }

            for (uint8_t led : leds) {
                digitalWrite(led, LOW);
            }
        });

        ArduinoOTA.onProgress([](uint32_t progress, uint32_t total) {
            // Use LEDs as progress bar
            uint8_t leds[LED_COUNT] = LED_ORDER;
            uint32_t ledThresh = total / LED_COUNT + 1;
            uint8_t curLed = progress / ledThresh;
            uint32_t ledProgress = map(progress % ledThresh, 0, ledThresh, 0, 255);

            for (int led = 0; led < curLed; led++) {
                digitalWrite(curLed, HIGH);
            }
            for (int led = curLed + 1; led < LED_COUNT; led++) {
                digitalWrite(curLed, LOW);
            }

            if (curLed < LED_COUNT) {
                analogWrite(leds[curLed], ledProgress);
            }
        });

        ArduinoOTA.onEnd([]() {
            debugf("OTA finished\r\nPlease wait for the atomic firmware copy\r\n");
            UART_DEBUG.flush();

            uint8_t leds[LED_COUNT] = LED_ORDER;
            digitalWrite(LED_WIFI, HIGH);
            digitalWrite(LED_WIFI, HIGH);

            if (LED_COUNT >= 3) {
                digitalWrite(leds[LED_COUNT - 1], LOW);
                delay(400);
                digitalWrite(leds[LED_COUNT - 2], LOW);
                delay(400);
                digitalWrite(leds[LED_COUNT - 3], LOW);
            }
        });

        ArduinoOTA.onError([](ota_error_t err) {
            debugf("OTA error: ");
            switch (err) {
                case OTA_AUTH_ERROR:
                    debugf("AUTH\r\n"); break;
                case OTA_BEGIN_ERROR:
                    debugf("BEGIN\r\n"); break;
                case OTA_CONNECT_ERROR:
                    debugf("CONNECT\r\n"); break;
                case OTA_RECEIVE_ERROR:
                    debugf("RECEIVE\r\n"); break;
                case OTA_END_ERROR:
                    debugf("END\r\n"); break;
            }

            // Blink red LEDs, then reset
            blinkError();
            ESP.reset();
        });

        ArduinoOTA.begin();
        debugf("OTA server is up\r\n");
    }

    MDNS.addService("http", "tcp", 80);
#endif
}

void loop() {
    ArduinoOTA.handle();
    if (otaRunning) {
        return yield();
    }

    if (WIFI_MODE == WIFI_STA && WiFi.status() != WL_CONNECTED) {
        blinkError();
        ESP.reset();
    }

    ttyd->dispatchUart();
    yield();
    ttyd->performHousekeeping();
    yield();
}