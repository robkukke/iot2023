#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <BearSSLHelpers.h>
#include "config.h"
#include "certs.h"

#define SERIAL_BAUD 115200
#define LED_PIN 7
#define LIGHT_SENSOR_PIN 0

BearSSL::WiFiClientSecure wifiClient;
BearSSL::X509List trustedRoots;

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);

    Serial.print("Connecting to Wi-Fi...");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.printf("\nConnected to Wi-Fi network: %s\n", SSID);
}

void setClock() {
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print("Waiting for NTP time sync...");

    time_t now = time(nullptr);

    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");

        now = time(nullptr);
    }

    struct tm timeInfo{};

    gmtime_r(&now, &timeInfo);

    Serial.printf("\nCurrent time (UTC): %s\n", asctime(&timeInfo));
}

void httpsRequest(const char *url, bool checkPayload = false) {
    HTTPClient https;

    if (https.begin(wifiClient, url)) {
        Serial.printf("[HTTPS] GET... URL: %s\n", url);

        int httpCode = https.GET();

        if (httpCode > 0) {
            Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

            if (checkPayload) {
                String payload = https.getString();
                payload = payload.substring(1, payload.length() - 1);

                Serial.printf("[HTTPS] GET... payload: %s\n", payload.c_str());

                digitalWrite(LED_PIN, (payload == "jah") ? HIGH : LOW);
            }
        } else {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", HTTPClient::errorToString(httpCode).c_str());
        }

        https.end();
    } else {
        Serial.printf("[HTTPS] Unable to connect to URL: %s\n", url);
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);

    trustedRoots.append(cert_GTS_Root_R1);
    wifiClient.setTrustAnchors(&trustedRoots);

    connectToWiFi();
    setClock();

    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        int brightness = analogRead(LIGHT_SENSOR_PIN);
        String brightnessDataUrl = BRIGHTNESS_DATA_URL_BASE + String(brightness);

        Serial.printf("Current brightness value: %d\n", brightness);

        httpsRequest(LIGHT_STATUS_URL, true);
        httpsRequest(brightnessDataUrl.c_str());
    }

    Serial.println("Wait 5s before the next round...");
    delay(5000);
}
