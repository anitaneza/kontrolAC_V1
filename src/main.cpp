#include <Arduino.h>
#include "config.h"
#include "DHTRoomSensor.h"
#include "WifiHandler.h"
#include "MQTTHandler.h"
#include "SPIFFSHandler.h"
#include "IRRawCodes.h"
#include "IRTransmitterACController.h"
#include "IRReceiverCapture.h"
#include "PIRSensor.h"
#include "FuzzyMamdani.h"
#include "HTTPLogger.h"
#include <ArduinoJson.h>

// ─── State ─────────────────────────────────────────────────────
enum SystemState {
    STATE_NORMAL,
    STATE_CONFIG,
    STATE_CAPTURING,
    STATE_WAITING_CONFIRM
};

SystemState currentState         = STATE_NORMAL;

// ─── IR Capture ────────────────────────────────────────────────
char     pendingCaptureKey[16]   = "";
uint16_t pendingRawBuf[200]      = {};
uint16_t pendingRawLen           = 0;

// ─── PIR & AC ──────────────────────────────────────────────────
bool          isOccupied          = false;
bool          isACOn              = false;
bool          isEmptyTimerActive  = false;
unsigned long emptyStartTime      = 0;

// ─── Fuzzy ─────────────────────────────────────────────────────
int           lastSetpoint        = -1;
unsigned long lastFuzzyTime       = 0;

// ─── Averaging Sensor ──────────────────────────────────────────
float         suhuAccum           = 0;
float         kelembabanAccum     = 0;
int           readCount           = 0;
unsigned long lastReadTime        = 0;
unsigned long lastSendTime        = 0;

// ─── Nilai Terakhir untuk StatusLog ────────────────────────────
String        lastPIRStatus       = "empty";
String        lastACStatus        = "off";
int           lastACSetpoint      = 0;
int           lastFuzzySetpoint   = 0;

// ─── Flag perubahan status ─────────────────────────────────────
bool          statusChanged       = false;

// ─── Inisialisasi Fuzzy dari config.h ──────────────────────────
TempMF tempMF = {
    { TEMP_DINGIN_A, TEMP_DINGIN_B, TEMP_DINGIN_C, TEMP_DINGIN_D },
    { TEMP_NYAMAN_A, TEMP_NYAMAN_B, TEMP_NYAMAN_C },
    { TEMP_PANAS_A,  TEMP_PANAS_B,  TEMP_PANAS_C,  TEMP_PANAS_D  }
};

HumidMF humidMF = {
    { HUMID_RENDAH_A, HUMID_RENDAH_B, HUMID_RENDAH_C, HUMID_RENDAH_D },
    { HUMID_SEDANG_A, HUMID_SEDANG_B, HUMID_SEDANG_C },
    { HUMID_TINGGI_A, HUMID_TINGGI_B, HUMID_TINGGI_C, HUMID_TINGGI_D }
};

SetpointMF setpointMF = {
    { SETPOINT_RENDAH_A, SETPOINT_RENDAH_B, SETPOINT_RENDAH_C, SETPOINT_RENDAH_D },
    { SETPOINT_SEDANG_A, SETPOINT_SEDANG_B, SETPOINT_SEDANG_C },
    { SETPOINT_TINGGI_A, SETPOINT_TINGGI_B, SETPOINT_TINGGI_C, SETPOINT_TINGGI_D }
};

RuleBase ruleBase = {{
    { RULE_00, RULE_01, RULE_02 },
    { RULE_10, RULE_11, RULE_12 },
    { RULE_20, RULE_21, RULE_22 }
}};

// ─── Objek ─────────────────────────────────────────────────────
DHTRoomSensor             roomSensor(DHT_PIN, DHT_TYPE);
WifiHandler               wifi(WIFI_SSID, WIFI_PASS);
MQTTHandler               mqtt(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
SPIFFSHandler             spiffs;
IRRawCodes                irCodes(spiffs, IR_JSON_PATH);
IRTransmitterACController irTx(IR_TX_PIN, irCodes);
IRReceiverCapture         irRx(IR_RX_PIN);
PIRSensor                 pir(PIR_PIN);
FuzzyMamdani              fuzzy(tempMF, humidMF, setpointMF, ruleBase);
HTTPLogger                httpLogger(APPS_SCRIPT_URL);

// ─── Fungsi bantu: kirim StatusLog ─────────────────────────────
void sendStatusLog() {
    // MQTT kirim kolom yang berubah
    mqtt.publish(TOPIC_PIR_STATUS,    lastPIRStatus.c_str());
    mqtt.publish(TOPIC_AC_STATUS,     lastACStatus.c_str());
    mqtt.publish(TOPIC_FUZZY_SETPOINT, (float)lastFuzzySetpoint, 0);

    // Apps Script kirim semua kolom
    httpLogger.sendStatusLog(
        SHEET_STATUS_LOG,
        lastPIRStatus.c_str(),
        lastACStatus.c_str(),
        lastACSetpoint,
        lastFuzzySetpoint
    );
    statusChanged = false;
}

// ─── Fungsi bantu AC ───────────────────────────────────────────
void turnACOn() {
    if (isACOn) return;
    irTx.sendKey("on");
    isACOn             = true;
    isEmptyTimerActive = false;
    lastSetpoint       = -1;
    lastACStatus       = "on";
    statusChanged      = true;
    mqtt.publish(TOPIC_AC_STATUS, "on");
    Serial.println("[AC] Dinyalakan.");
}

void turnACOff() {
    if (!isACOn) return;
    irTx.sendKey("off");
    isACOn             = false;
    isEmptyTimerActive = false;
    lastACStatus       = "off";
    lastACSetpoint     = 0;
    statusChanged      = true;
    mqtt.publish(TOPIC_AC_STATUS, "off");
    Serial.println("[AC] Dimatikan.");
}

// ─── MQTT Callback ─────────────────────────────────────────────
void onMqttMessage(const char* topic, const char* payload) {
    Serial.printf("[MQTT] Terima | topic: %s | payload: %s\n", topic, payload);

    if (strcmp(topic, TOPIC_MODE) == 0) {
        if (strcmp(payload, "config") == 0 && currentState == STATE_NORMAL) {
            currentState = STATE_CONFIG;
            Serial.println("[State] → CONFIG");
        } else if (strcmp(payload, "normal") == 0) {
            if (currentState == STATE_CAPTURING) irRx.stop();
            currentState = STATE_NORMAL;
            Serial.println("[State] → NORMAL");
        }
        return;
    }

    if (strcmp(topic, TOPIC_CAPTURE) == 0 && currentState == STATE_CONFIG) {
        strncpy(pendingCaptureKey, payload, sizeof(pendingCaptureKey) - 1);
        currentState = STATE_CAPTURING;
        irRx.begin();
        Serial.printf("[State] → CAPTURING key: %s\n", pendingCaptureKey);
        return;
    }

    if (strcmp(topic, TOPIC_CAPTURE_CONFIRM) == 0 && currentState == STATE_WAITING_CONFIRM) {
        if (strcmp(payload, pendingCaptureKey) == 0) {
            irCodes.update(pendingCaptureKey, pendingRawBuf, pendingRawLen);
            Serial.printf("[IRRawCodes] Key '%s' disimpan ke SPIFFS.\n", pendingCaptureKey);

            // Kirim ke IRLog Apps Script
            // httpLogger.sendIRLog(SHEET_IR_LOG, pendingCaptureKey, "IR raw updated");
            // Kirim ke IRLog Apps Script dengan raw data
            httpLogger.sendIRLog(
                SHEET_IR_LOG,
                pendingCaptureKey,
                pendingRawBuf,
                pendingRawLen,
                "IR raw updated"
            );
        } else {
            Serial.println("[IRRawCodes] Konfirmasi tidak cocok, data diabaikan.");
        }
        memset(pendingCaptureKey, 0, sizeof(pendingCaptureKey));
        pendingRawLen = 0;
        currentState  = STATE_CONFIG;
        Serial.println("[State] → CONFIG");
        return;
    }
}

void onMqttReconnect() {
    mqtt.subscribe(TOPIC_MODE);
    mqtt.subscribe(TOPIC_CAPTURE);
    mqtt.subscribe(TOPIC_CAPTURE_CONFIRM);
}

// ─── Setup ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    spiffs.begin();
    irCodes.load();

    roomSensor.begin();
    pir.begin();
    wifi.connect();

    mqtt.setCallback(onMqttMessage);
    mqtt.setReconnectCallback(onMqttReconnect);
    mqtt.connect();

    irTx.begin();

    lastReadTime = millis();
    lastSendTime = millis();

    Serial.println("Sistem siap. State: NORMAL");
}

// ─── Loop ──────────────────────────────────────────────────────
void loop() {
    mqtt.loop();

    // ── STATE: NORMAL ──────────────────────────────────────────
    if (currentState == STATE_NORMAL) {
        unsigned long now = millis();

        // Baca sensor setiap 1 detik
        if (now - lastReadTime >= DATA_READ_INTERVAL_MS) {
            lastReadTime = now;

            float suhu       = roomSensor.readTemperature();
            float kelembaban = roomSensor.readHumidity();

            if (roomSensor.isValid()) {
                // Kalibrasi
                suhu       += DHT_TEMP_OFFSET;
                kelembaban += DHT_HUMID_OFFSET;

                suhuAccum      += suhu;
                kelembabanAccum += kelembaban;
                readCount++;
            }
        }

        // Kirim rata-rata setiap 1 menit
        if (now - lastSendTime >= DATA_SEND_INTERVAL_MS) {
            lastSendTime = now;

            if (readCount > 0) {
                float suhuAvg       = suhuAccum / readCount;
                float kelembabanAvg = kelembabanAccum / readCount;

                // Reset akumulator
                suhuAccum       = 0;
                kelembabanAccum = 0;
                readCount       = 0;

                // Publish MQTT
                mqtt.publish(TOPIC_ROOM_TEMP,  suhuAvg);
                mqtt.publish(TOPIC_ROOM_HUMID, kelembabanAvg);

                // Kirim ke Apps Script
                httpLogger.sendSensorLog(SHEET_SENSOR_LOG, suhuAvg, kelembabanAvg);

                Serial.printf("[Sensor] Avg Suhu: %.2f°C | Avg Kelembaban: %.2f%%\n",
                              suhuAvg, kelembabanAvg);
            }
        }

        // Logika PIR
        bool pirDetected = pir.isDetected();

        if (pirDetected && !isOccupied) {
            isOccupied         = true;
            isEmptyTimerActive = false;
            lastPIRStatus      = "detected";
            statusChanged      = true;
            Serial.println("[PIR] Orang terdeteksi.");
            turnACOn();

        } else if (!pirDetected && isOccupied) {
            isOccupied         = false;
            isEmptyTimerActive = true;
            emptyStartTime     = now;
            lastPIRStatus      = "empty";
            statusChanged      = true;
            Serial.println("[PIR] Ruangan kosong, timer mulai.");

        } else if (!pirDetected && isEmptyTimerActive) {
            if (now - emptyStartTime >= AC_OFF_DELAY_MS) {
                isEmptyTimerActive = false;
                Serial.println("[PIR] Timer habis, matikan AC.");
                turnACOff();
            }
        }

        // Kirim StatusLog kalau ada perubahan
        if (statusChanged) {
            sendStatusLog();
        }

        // Fuzzy setiap 10 detik, hanya kalau AC menyala
        if (isACOn && (now - lastFuzzyTime >= FUZZY_INTERVAL_MS)) {
            lastFuzzyTime = now;

            float suhu       = roomSensor.readTemperature();
            float kelembaban = roomSensor.readHumidity();

            if (roomSensor.isValid()) {
                suhu       += DHT_TEMP_OFFSET;
                kelembaban += DHT_HUMID_OFFSET;

                FuzzyResult result = fuzzy.compute(suhu, kelembaban);

                Serial.printf("[Fuzzy] Suhu: %.2f | Kelembaban: %.2f\n", suhu, kelembaban);
                Serial.printf("[Fuzzy] Firing → Rendah: %.2f | Sedang: %.2f | Tinggi: %.2f\n",
                              result.firingRendah, result.firingSedang, result.firingTinggi);
                Serial.printf("[Fuzzy] Setpoint crisp: %.2f → %d°C\n",
                              result.crispSetpoint, result.setpointInt);

                // Kirim ke AC dan log hanya kalau setpoint berubah
                if (result.setpointInt != lastSetpoint) {
                    char keyBuf[4];
                    snprintf(keyBuf, sizeof(keyBuf), "%d", result.setpointInt);
                    irTx.sendKey(keyBuf);

                    lastSetpoint      = result.setpointInt;
                    lastACSetpoint    = result.setpointInt;
                    lastFuzzySetpoint = result.setpointInt;
                    statusChanged     = true;

                    Serial.printf("[IR] Setpoint dikirim: %d°C\n", result.setpointInt);

                    // Publish firing ke MQTT
                    mqtt.publish(TOPIC_FUZZY_FIRING_LOW,  result.firingRendah);
                    mqtt.publish(TOPIC_FUZZY_FIRING_MID,  result.firingSedang);
                    mqtt.publish(TOPIC_FUZZY_FIRING_HIGH, result.firingTinggi);
                }
            } else {
                Serial.println("[Fuzzy] Sensor gagal baca, fuzzy dilewati.");
            }
        }
    }

    // ── STATE: CAPTURING ───────────────────────────────────────
    if (currentState == STATE_CAPTURING) {
        if (irRx.capture(pendingRawBuf, pendingRawLen)) {
            irRx.stop();

            JsonDocument doc;
            JsonArray arr = doc["raw"].to<JsonArray>();
            for (uint16_t i = 0; i < pendingRawLen; i++) arr.add(pendingRawBuf[i]);
            doc["key"] = pendingCaptureKey;

            char jsonStr[2048];
            serializeJson(doc, jsonStr, sizeof(jsonStr));
            mqtt.publish(TOPIC_CAPTURE_RESULT, jsonStr);

            Serial.printf("[IRReceiver] Capture selesai, %d sinyal. Menunggu konfirmasi...\n",
                          pendingRawLen);
            currentState = STATE_WAITING_CONFIRM;
        }
    }
}