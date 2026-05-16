#include "SPIFFSHandler.h"

bool SPIFFSHandler::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[SPIFFS] Gagal mount.");
        return false;
    }
    Serial.println("[SPIFFS] Mount berhasil.");
    return true;
}

bool SPIFFSHandler::loadJson(const char* path, JsonDocument& doc) {
    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.printf("[SPIFFS] Gagal buka file: %s\n", path);
        return false;
    }

    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        Serial.printf("[SPIFFS] Gagal parse JSON: %s\n", err.c_str());
        return false;
    }
    return true;
}

bool SPIFFSHandler::saveJson(const char* path, JsonDocument& doc) {
    File file = SPIFFS.open(path, "w");
    if (!file) {
        Serial.printf("[SPIFFS] Gagal buka file untuk tulis: %s\n", path);
        return false;
    }

    serializeJson(doc, file);
    file.close();
    Serial.printf("[SPIFFS] File disimpan: %s\n", path);
    return true;
}