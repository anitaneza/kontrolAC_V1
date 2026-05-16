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
#include <ArduinoJson.h>

// ─── State ─────────────────────────────────────────────────────
enum SystemState {
    STATE_NORMAL,
    STATE_CONFIG,
    STATE_CAPTURING,
    STATE_WAITING_CONFIRM
};

SystemState currentState     = STATE_NORMAL;

// ─── IR Capture ────────────────────────────────────────────────
char     pendingCaptureKey[16] = "";
uint16_t pendingRawBuf[200]    = {};
uint16_t pendingRawLen         = 0;

// ─── PIR & AC ──────────────────────────────────────────────────
bool          isOccupied        = false;
bool          isACOn            = false;
unsigned long emptyStartTime    = 0;
bool          isEmptyTimerActive = false;

// ─── Objek ─────────────────────────────────────────────────────
DHTRoomSensor             roomSensor(DHT_PIN, DHT_TYPE);
WifiHandler               wifi(WIFI_SSID, WIFI_PASS);
MQTTHandler               mqtt(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
SPIFFSHandler             spiffs;
IRRawCodes                irCodes(spiffs, IR_JSON_PATH);
IRTransmitterACController irTx(IR_TX_PIN, irCodes);
IRReceiverCapture         irRx(IR_RX_PIN);
PIRSensor                 pir(PIR_PIN);

// ─── Interval publish sensor ───────────────────────────────────
static const unsigned long PUBLISH_INTERVAL_MS = 5000;
unsigned long lastPublishTime = 0;

// ─── Fungsi bantu AC ───────────────────────────────────────────
void turnACOn() {
    if (isACOn) return;
    irTx.sendKey("on");
    isACOn = true;
    isEmptyTimerActive = false;
    mqtt.publish(TOPIC_AC_STATUS, "on");
    Serial.println("[AC] Dinyalakan.");
}

void turnACOff() {
    if (!isACOn) return;
    irTx.sendKey("off");
    isACOn = false;
    isEmptyTimerActive = false;
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

    roomSensor.mulai();
    pir.begin();
    wifi.connect();

    mqtt.setCallback(onMqttMessage);
    mqtt.setReconnectCallback(onMqttReconnect);
    mqtt.connect();

    irTx.begin();

    Serial.println("Sistem siap. State: NORMAL");
}

// ─── Loop ──────────────────────────────────────────────────────
void loop() {
    mqtt.loop();

    // ── STATE: NORMAL ──────────────────────────────────────────
    if (currentState == STATE_NORMAL) {

        // Publish sensor DHT
        unsigned long now = millis();
        if (now - lastPublishTime >= PUBLISH_INTERVAL_MS) {
            lastPublishTime = now;

            float suhu       = roomSensor.bacaTemperature();
            float kelembaban = roomSensor.bacaHumidity();

            if (roomSensor.isValid()) {
                mqtt.publish(TOPIC_ROOM_TEMP,  suhu);
                mqtt.publish(TOPIC_ROOM_HUMID, kelembaban);
                Serial.printf("[Sensor] Suhu: %.2f°C | Kelembaban: %.2f%%\n", suhu, kelembaban);
            } else {
                Serial.println("[Sensor] Gagal baca, data tidak dikirim.");
            }
        }

        // Logika PIR
        bool pirDetected = pir.isDetected();

        if (pirDetected && !isOccupied) {
            // Ada orang masuk
            isOccupied = true;
            isEmptyTimerActive = false;
            mqtt.publish(TOPIC_PIR_STATUS, "detected");
            Serial.println("[PIR] Orang terdeteksi.");
            turnACOn();

        } else if (!pirDetected && isOccupied) {
            // Orang baru saja pergi
            isOccupied = false;
            isEmptyTimerActive = true;
            emptyStartTime = millis();
            mqtt.publish(TOPIC_PIR_STATUS, "empty");
            Serial.println("[PIR] Ruangan kosong, timer mulai.");

        } else if (!pirDetected && isEmptyTimerActive) {
            // Ruangan masih kosong, cek timer
            if (millis() - emptyStartTime >= AC_OFF_DELAY_MS) {
                isEmptyTimerActive = false;
                Serial.println("[PIR] Timer habis, matikan AC.");
                turnACOff();
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
            mqtt.publish(TOPIC_CAPTURE_RESULT, "Berhasil Terdeteksi");

            Serial.printf("[IRReceiver] Capture selesai, %d sinyal. Menunggu konfirmasi...\n", pendingRawLen);
            currentState = STATE_WAITING_CONFIRM;
        }
    }
}