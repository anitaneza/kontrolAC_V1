#include "IRRawCodes.h"

IRRawCodes::IRRawCodes(SPIFFSHandler& spiffs, const char* jsonPath)
    : _spiffs(spiffs), _jsonPath(jsonPath)
{
}

bool IRRawCodes::load() {
    bool ok = _spiffs.loadJson(_jsonPath, _doc);
    if (ok) Serial.println("[IRRawCodes] Kode raw berhasil dimuat.");
    return ok;
}

bool IRRawCodes::get(const char* key, uint16_t* outBuf, uint16_t& outLen) {
    if (!_doc[key].is<JsonArray>()) {
        Serial.printf("[IRRawCodes] Key tidak ditemukan: %s\n", key);
        return false;
    }

    JsonArray arr = _doc[key].as<JsonArray>();
    outLen = 0;
    for (uint16_t val : arr) {
        outBuf[outLen++] = val;
        if (outLen >= MAX_RAW_LEN) break;
    }
    return true;
}

bool IRRawCodes::update(const char* key, const uint16_t* rawData, uint16_t len) {
    JsonArray arr = _doc[key].to<JsonArray>();
    for (uint16_t i = 0; i < len; i++) {
        arr.add(rawData[i]);
    }
    return _spiffs.saveJson(_jsonPath, _doc);
}

JsonDocument& IRRawCodes::getDocument() {
    return _doc;
}