#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SPIFFSHandler.h"

#define MAX_RAW_LEN 200

class IRRawCodes {
public:
    IRRawCodes(SPIFFSHandler& spiffs, const char* jsonPath);

    bool load();
    bool get(const char* key, uint16_t* outBuf, uint16_t& outLen);
    bool update(const char* key, const uint16_t* rawData, uint16_t len);
    
    JsonDocument& getDocument();

private:
    SPIFFSHandler& _spiffs;
    const char*    _jsonPath;
    JsonDocument   _doc;
};