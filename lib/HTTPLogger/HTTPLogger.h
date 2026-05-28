#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class HTTPLogger {
public:
    HTTPLogger(const char* scriptUrl);

    bool sendSensorLog(const char* sheetName, float suhuAvg, float kelembabanAvg);
    bool sendStatusLog(const char* sheetName, const char* pirStatus,
                       const char* acStatus, int acSetpoint, int fuzzySetpoint);
    bool sendIRLog(const char* sheetName, const char* key, const char* keterangan);

private:
    const char* _scriptUrl;
    bool        _post(JsonDocument& doc);
};