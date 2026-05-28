#include "HTTPLogger.h"

HTTPLogger::HTTPLogger(const char* scriptUrl)
    : _scriptUrl(scriptUrl)
{
}

bool HTTPLogger::_post(JsonDocument& doc) {
    HTTPClient http;
    http.begin(_scriptUrl);
    http.addHeader("Content-Type", "application/json");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    char jsonStr[512];
    serializeJson(doc, jsonStr, sizeof(jsonStr));

    int httpCode = http.POST(jsonStr);

    if (httpCode == 200 || httpCode == 302) {
        http.end();
        return true;
    }

    Serial.printf("[HTTP] Gagal rc=%d, retry...\n", httpCode);
    http.end();
    delay(3000);

    http.begin(_scriptUrl);
    http.addHeader("Content-Type", "application/json");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpCode = http.POST(jsonStr);
    http.end();

    if (httpCode == 200 || httpCode == 302) return true;

    Serial.printf("[HTTP] Retry gagal rc=%d, data dibuang.\n", httpCode);
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

bool HTTPLogger::sendIRLog(const char* sheetName, const char* key, const char* keterangan) {
    JsonDocument doc;
    doc["sheet"]      = sheetName;
    doc["key"]        = key;
    doc["keterangan"] = keterangan;
    Serial.println("[HTTP] Kirim IRLog...");
    return _post(doc);
}