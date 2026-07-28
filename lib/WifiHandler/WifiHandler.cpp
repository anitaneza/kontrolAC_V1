#include "WifiHandler.h"
#include <Arduino.h>
#include <WiFi.h>

WifiHandler::WifiHandler(const char* ssid, const char* password)
    : _ssid(ssid), _password(password)
{
}

void WifiHandler::begin() {
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);

    connect();
}

void WifiHandler::connect() {
    Serial.print("[WiFi] Menghubungkan ke: ");
    Serial.println(_ssid);

    WiFi.begin(_ssid, _password);
    waitUntilConnected();
}

void WifiHandler::waitUntilConnected() {
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("[WiFi] Terhubung. IP: ");
    Serial.println(WiFi.localIP());
}

bool WifiHandler::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WifiHandler::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        static unsigned long lastTry = 0;
        unsigned long now = millis();

        if (now - lastTry >= 5000) {
            lastTry = now;
            Serial.println("[WiFi] Koneksi putus, mencoba reconnect...");
            WiFi.reconnect();
        }
    }
}