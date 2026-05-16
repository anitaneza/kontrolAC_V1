#include "WifiHandler.h"
#include <Arduino.h>
#include <WiFi.h>

WifiHandler::WifiHandler(const char* ssid, const char* password)
    : _ssid(ssid), _password(password)
{
}

void WifiHandler::connect() {
    Serial.print("Menghubungkan ke WiFi: ");
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
    Serial.print("WiFi terhubung. IP: ");
    Serial.println(WiFi.localIP());
}

bool WifiHandler::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}