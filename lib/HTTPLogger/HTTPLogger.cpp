#include "HTTPLogger.h"
#include <WiFiClientSecure.h>

HTTPLogger::HTTPLogger(const char* scriptUrl)
    : _scriptUrl(scriptUrl)
{
}


bool HTTPLogger::_post(JsonDocument& doc) {
    delay(100);
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, _scriptUrl);
    http.addHeader("Content-Type", "application/json");
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

    String jsonStr;
    serializeJson(doc, jsonStr);

    // Serial.println("[HTTP] Payload: " + jsonStr);

    int httpCode = http.POST(jsonStr);

    // Serial.printf("[HTTP] rc=%d\n", httpCode);
    // Serial.println("[HTTP] Response: " + http.getString());

    http.end();

    if (httpCode == 200 || httpCode == 302) {
        Serial.println("[HTTP] Berhasil.");
        return true;
    }

    Serial.printf("[HTTP] Gagal rc=%d\n", httpCode);
    return false;
}

bool HTTPLogger::sendSensorLog(const char* sheetName, float suhuAvg, float kelembabanAvg) {
    JsonDocument doc;
    doc["sheet"]     = sheetName;
    doc["suhu_avg"]  = suhuAvg;
    doc["humid_avg"] = kelembabanAvg;
    Serial.println("[HTTP] Kirim SensorLog...");
    return _post(doc);
}

bool HTTPLogger::sendStatusLog(const char* sheetName, const char* pirStatus,
                                const char* acStatus, int acSetpoint, int fuzzySetpoint) {
    JsonDocument doc;
    doc["sheet"]          = sheetName;
    doc["pir_status"]     = pirStatus;
    doc["ac_status"]      = acStatus;
    doc["ac_setpoint"]    = acSetpoint;
    doc["fuzzy_setpoint"] = fuzzySetpoint;
    Serial.println("[HTTP] Kirim StatusLog...");
    return _post(doc);
}

bool HTTPLogger::sendIRLog(const char* sheetName, const char* key,
                            const uint16_t* rawBuf, uint16_t rawLen,
                            const char* keterangan) {
    JsonDocument doc;
    doc["sheet"]      = sheetName;
    doc["key"]        = key;
    doc["keterangan"] = keterangan;

    JsonArray arr = doc["raw"].to<JsonArray>();
    for (uint16_t i = 0; i < rawLen; i++) {
        arr.add(rawBuf[i]);
    }

    Serial.println("[HTTP] Kirim IRLog...");
    return _post(doc);
}