#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class SPIFFSHandler {
public:
    bool begin();
    bool loadJson(const char* path, JsonDocument& doc);
    bool saveJson(const char* path, JsonDocument& doc);
};