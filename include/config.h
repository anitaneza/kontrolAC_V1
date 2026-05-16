#pragma once

// ─── DHT Room Sensor ───────────────────────────────────────────
#define DHT_PIN     4
#define DHT_TYPE    DHT22

// ─── IR ────────────────────────────────────────────────────────
#define IR_TX_PIN   5
#define IR_RX_PIN   18
#define IR_RAW_LEN  139

// ─── PIR Sensor ────────────────────────────────────────────────
#define PIR_PIN     14

// ─── AC Auto Off Timer ─────────────────────────────────────────
#define AC_OFF_DELAY_MS         (15UL * 60UL * 1000UL)  // 15 menit

// ─── WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID   "backup"
#define WIFI_PASS   "backup1234"

// ─── MQTT Broker ───────────────────────────────────────────────
#define MQTT_BROKER   "broker.hivemq.com"  
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "Client2204129"  

// ─── MQTT Topics ESP1 ──────────────────────────────────────────
#define TOPIC_ROOM_TEMP        "2204129/esp1/dht/room/temperature"
#define TOPIC_ROOM_HUMID       "2204129/esp1/dht/room/humidity"
#define TOPIC_MODE             "2204129/esp1/mode"
#define TOPIC_CAPTURE          "2204129/esp1/capture"
#define TOPIC_CAPTURE_RESULT   "2204129/esp1/capture/result"
#define TOPIC_CAPTURE_CONFIRM  "2204129/esp1/capture/confirm"
#define TOPIC_PIR_STATUS        "2204129/esp1/pir/status"
#define TOPIC_AC_STATUS         "2204129/esp1/ac/status"

// ─── SPIFFS ────────────────────────────────────────────────────
#define IR_JSON_PATH  "/irRawCodes.json"