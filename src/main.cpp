#include <Arduino.h>
#include "config.h"
#include "DHTRoomSensor.h"
#include "WifiHandler.h"
#include "MQTTHandler.h"
#include "SPIFFSHandler.h"
#include "IRRawCodes.h"
#include "IRTransmitterACController.h"
#include "IRReceiverCapture.h"
#include "OccupancyCounter.h"
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

// ─── AC Mode ────────────────────────────────────────────────────
enum ACMode {
    AC_AUTO,
    AC_MANUAL
};
ACMode acMode = AC_AUTO;

// ─── IR Capture ────────────────────────────────────────────────
char     pendingCaptureKey[16]   = "";
uint16_t pendingRawBuf[CAPTURE_BUF_SIZE]      = {};
uint16_t pendingRawLen           = 0;

// ─── Occupancy & AC ────────────────────────────────────────────
int           lastOccupancyCount  = 0;
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
String        lastPIRStatus       = "0";
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
    { HUMID_KERING_A, HUMID_KERING_B, HUMID_KERING_C, HUMID_KERING_D },
    { HUMID_NORMAL_A, HUMID_NORMAL_B, HUMID_NORMAL_C },
    { HUMID_LEMBAB_A, HUMID_LEMBAB_B, HUMID_LEMBAB_C, HUMID_LEMBAB_D }
};

OccupancyMF occMF = {
    { OCC_SEDIKIT_A, OCC_SEDIKIT_B, OCC_SEDIKIT_C, OCC_SEDIKIT_D },
    { OCC_SEDANG_A,  OCC_SEDANG_B,  OCC_SEDANG_C  },
    { OCC_BANYAK_A,  OCC_BANYAK_B,  OCC_BANYAK_C,  OCC_BANYAK_D  }
};

SetpointMF setpointMF = {
    { SETPOINT_RENDAH_A, SETPOINT_RENDAH_B, SETPOINT_RENDAH_C, SETPOINT_RENDAH_D },
    { SETPOINT_SEDANG_A, SETPOINT_SEDANG_B, SETPOINT_SEDANG_C },
    { SETPOINT_TINGGI_A, SETPOINT_TINGGI_B, SETPOINT_TINGGI_C, SETPOINT_TINGGI_D }
};

RuleBase ruleBase = {{
    {
        { RULE_000, RULE_001, RULE_002 },
        { RULE_010, RULE_011, RULE_012 },
        { RULE_020, RULE_021, RULE_022 }
    },
    {
        { RULE_100, RULE_101, RULE_102 },
        { RULE_110, RULE_111, RULE_112 },
        { RULE_120, RULE_121, RULE_122 }
    },
    {
        { RULE_200, RULE_201, RULE_202 },
        { RULE_210, RULE_211, RULE_212 },
        { RULE_220, RULE_221, RULE_222 }
    }
}};

// ─── Objek ─────────────────────────────────────────────────────
DHTRoomSensor             roomSensor(DHT_PIN, DHT_TYPE);
WifiHandler               wifi(WIFI_SSID, WIFI_PASS);
MQTTHandler               mqtt(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
SPIFFSHandler             spiffs;
IRRawCodes                irCodes(spiffs, IR_JSON_PATH);
IRTransmitterACController irTx(IR_TX_PIN, irCodes);
IRReceiverCapture         irRx(IR_RX_PIN);
OccupancyCounter          occupancy(IR_FRONT_PIN, IR_BACK_PIN, IR_SENSOR_TIMEOUT_MS);
FuzzyMamdani              fuzzy(tempMF, humidMF, occMF, setpointMF, ruleBase);
HTTPLogger                httpLogger(APPS_SCRIPT_URL);

// ─── Fungsi bantu: kirim StatusLog ─────────────────────────────
void sendStatusLog() {
    mqtt.publish(TOPIC_PIR_STATUS,     lastPIRStatus.c_str());
    mqtt.publish(TOPIC_AC_STATUS,      lastACStatus.c_str());
    mqtt.publish(TOPIC_FUZZY_SETPOINT, (float)lastFuzzySetpoint, 0);

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

    if (strcmp(topic, TOPIC_AC_MODE) == 0) {
        if (strcmp(payload, "auto") == 0) {
            acMode = AC_AUTO;
            mqtt.publish(TOPIC_AC_MODE, "berhasil auto");
            Serial.println("[AC Mode] → AUTO");
        } else if (strcmp(payload, "manual") == 0) {
            acMode = AC_MANUAL;
            mqtt.publish(TOPIC_AC_MODE, "berhasil manual");
            Serial.println("[AC Mode] → MANUAL");
        }
        return;
    }

    if (strcmp(topic, TOPIC_AC_POWER) == 0 && acMode == AC_MANUAL) {
        if (strcmp(payload, "on") == 0) {
            turnACOn();
            mqtt.publish(TOPIC_AC_POWER, "ok");
        } else if (strcmp(payload, "off") == 0) {
            turnACOff();
            mqtt.publish(TOPIC_AC_POWER, "ok");
        }
        return;
    }

    if (strcmp(topic, TOPIC_AC_SETPOINT) == 0 && acMode == AC_MANUAL) {
        int setpoint = atoi(payload);
        if (setpoint >= 16 && setpoint <= 30) {
            char keyBuf[4];
            snprintf(keyBuf, sizeof(keyBuf), "%d", setpoint);
            irTx.sendKey(keyBuf);

            lastSetpoint      = setpoint;
            lastACSetpoint    = setpoint;
            lastFuzzySetpoint = setpoint;
            lastFuzzyTime     = millis();
            statusChanged     = true;

            mqtt.publish(TOPIC_AC_SETPOINT, "ok");
            mqtt.publish(TOPIC_FUZZY_SETPOINT, (float)setpoint, 0);
            Serial.printf("[Manual] Setpoint AC: %d°C\n", setpoint);
        }
        return;
    }

    if (strcmp(topic, TOPIC_AC_OCCUPANCY) == 0) {
        int occ = atoi(payload);
        if (occ >= 0 && occ <= 10) {
            occupancy.setCount(occ);
            lastOccupancyCount = occ;
            lastPIRStatus      = String(occ);
            statusChanged      = true;
            mqtt.publish(TOPIC_AC_OCCUPANCY_RSP, "ok");
            Serial.printf("[OCC] Override manual: %d\n", occ);
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
    mqtt.subscribe(TOPIC_AC_MODE);
    mqtt.subscribe(TOPIC_AC_POWER);
    mqtt.subscribe(TOPIC_AC_SETPOINT);
    mqtt.subscribe(TOPIC_AC_OCCUPANCY);
}

// ─── Setup ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    spiffs.begin();
    irCodes.load();

    roomSensor.begin();
    occupancy.begin();
    wifi.begin();

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
    wifi.loop();
    mqtt.loop();

    // ── STATE: NORMAL ──────────────────────────────────────────
    if (currentState == STATE_NORMAL) {
        unsigned long now = millis();

        // Update occupancy counter setiap loop
        occupancy.update();
        int currentCount = occupancy.getCount();

        // Logika AC berdasarkan occupancy (hanya mode AUTO)
        if (acMode == AC_AUTO) {
            if (currentCount > 0 && !isACOn) {
                isEmptyTimerActive = false;
                turnACOn();
            } else if (currentCount == 0 && isACOn && !isEmptyTimerActive) {
                isEmptyTimerActive = true;
                emptyStartTime     = now;
                Serial.println("[OCC] Ruangan kosong, timer mulai.");
            } else if (currentCount == 0 && isEmptyTimerActive) {
                if (now - emptyStartTime >= AC_OFF_DELAY_MS) {
                    isEmptyTimerActive = false;
                    Serial.println("[OCC] Timer habis, matikan AC.");
                    turnACOff();
                }
            } else if (currentCount > 0 && isEmptyTimerActive) {
                isEmptyTimerActive = false;
                Serial.println("[OCC] Ada orang lagi, timer direset.");
            }
        }

        // Cek perubahan jumlah occupancy untuk publish
        if (currentCount != lastOccupancyCount) {
            lastOccupancyCount = currentCount;
            lastPIRStatus      = String(currentCount);
            statusChanged      = true;
            Serial.printf("[OCC] Count berubah: %d\n", currentCount);
        }

        // Baca sensor setiap 1 detik
        if (now - lastReadTime >= DATA_READ_INTERVAL_MS) {
            lastReadTime = now;

            float suhu       = roomSensor.readTemperature();
            float kelembaban = roomSensor.readHumidity();

            if (roomSensor.isValid()) {
                suhu       += DHT_TEMP_OFFSET;
                kelembaban += DHT_HUMID_OFFSET;

                suhuAccum       += suhu;
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

                suhuAccum       = 0;
                kelembabanAccum = 0;
                readCount       = 0;

                mqtt.publish(TOPIC_ROOM_TEMP,  suhuAvg);
                mqtt.publish(TOPIC_ROOM_HUMID, kelembabanAvg);

                httpLogger.sendSensorLog(SHEET_SENSOR_LOG, suhuAvg, kelembabanAvg);

                Serial.printf("[Sensor] Avg Suhu: %.2f°C | Avg Kelembaban: %.2f%%\n",
                              suhuAvg, kelembabanAvg);
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

                FuzzyResult result = fuzzy.compute(suhu, kelembaban, (float)currentCount);

                Serial.printf("[Fuzzy] Suhu: %.2f | Kelembaban: %.2f | Orang: %d\n",
                              suhu, kelembaban, currentCount);
                Serial.printf("[Fuzzy] Firing → Rendah: %.2f | Sedang: %.2f | Tinggi: %.2f\n",
                              result.firingRendah, result.firingSedang, result.firingTinggi);
                Serial.printf("[Fuzzy] Setpoint crisp: %.2f → %d°C\n",
                              result.crispSetpoint, result.setpointInt);

                if (result.setpointInt != lastSetpoint) {
                    lastSetpoint      = result.setpointInt;
                    lastACSetpoint    = result.setpointInt;
                    lastFuzzySetpoint = result.setpointInt;
                    statusChanged     = true;

                    if (acMode == AC_AUTO) {
                        char keyBuf[4];
                        snprintf(keyBuf, sizeof(keyBuf), "%d", result.setpointInt);
                        irTx.sendKey(keyBuf);
                        Serial.printf("[IR] Setpoint dikirim: %d°C\n", result.setpointInt);
                    }

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