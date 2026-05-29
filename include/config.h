#pragma once

// ─── DHT Room Sensor ───────────────────────────────────────────
#define DHT_PIN     4
#define DHT_TYPE    DHT22

// ─── IR ────────────────────────────────────────────────────────
#define IR_TX_PIN   15
#define IR_RX_PIN   18
#define IR_RAW_LEN  139

// ─── PIR Sensor ────────────────────────────────────────────────
#define PIR_PIN     14

// ─── Fuzzy Mamdani: Membership Function Suhu (°C) ──────────────
// Trapesium kiri  : [a, b, c, d]
// Segitiga        : [a, b, c]
// Trapesium kanan : [a, b, c, d]
#define TEMP_DINGIN_A   20.0f
#define TEMP_DINGIN_B   20.0f
#define TEMP_DINGIN_C   23.0f
#define TEMP_DINGIN_D   25.0f

#define TEMP_NYAMAN_A   23.0f
#define TEMP_NYAMAN_B   25.0f
#define TEMP_NYAMAN_C   27.0f

#define TEMP_PANAS_A    25.0f
#define TEMP_PANAS_B    27.0f
#define TEMP_PANAS_C    35.0f
#define TEMP_PANAS_D    35.0f

// ─── Fuzzy Mamdani: Membership Function Kelembaban (%) ─────────
#define HUMID_RENDAH_A  40.0f
#define HUMID_RENDAH_B  40.0f
#define HUMID_RENDAH_C  55.0f
#define HUMID_RENDAH_D  65.0f

#define HUMID_SEDANG_A  55.0f
#define HUMID_SEDANG_B  65.0f
#define HUMID_SEDANG_C  75.0f

#define HUMID_TINGGI_A  65.0f
#define HUMID_TINGGI_B  75.0f
#define HUMID_TINGGI_C  90.0f
#define HUMID_TINGGI_D  90.0f

// ─── Fuzzy Mamdani: Membership Function Output Setpoint (°C) ───
#define SETPOINT_RENDAH_A   16.0f
#define SETPOINT_RENDAH_B   16.0f
#define SETPOINT_RENDAH_C   18.0f
#define SETPOINT_RENDAH_D   23.0f

#define SETPOINT_SEDANG_A   18.0f
#define SETPOINT_SEDANG_B   23.0f
#define SETPOINT_SEDANG_C   28.0f

#define SETPOINT_TINGGI_A   23.0f
#define SETPOINT_TINGGI_B   28.0f
#define SETPOINT_TINGGI_C   30.0f
#define SETPOINT_TINGGI_D   30.0f

// ─── Fuzzy Mamdani: Rule Base ──────────────────────────────────
// rules[suhu][kelembaban] → 0=Rendah, 1=Sedang, 2=Tinggi
// suhu:      0=Dingin, 1=Nyaman, 2=Panas
// kelembaban: 0=Rendah, 1=Sedang, 2=Tinggi
#define RULE_00  2   // Dingin + Rendah  → Tinggi
#define RULE_01  2   // Dingin + Sedang  → Tinggi
#define RULE_02  1   // Dingin + Tinggi  → Sedang
#define RULE_10  2   // Nyaman + Rendah  → Tinggi
#define RULE_11  1   // Nyaman + Sedang  → Sedang
#define RULE_12  0   // Nyaman + Tinggi  → Rendah
#define RULE_20  1   // Panas  + Rendah  → Sedang
#define RULE_21  0   // Panas  + Sedang  → Rendah
#define RULE_22  0   // Panas  + Tinggi  → Rendah

// ─── Apps Script ───────────────────────────────────────────────
#define APPS_SCRIPT_URL   "https://script.google.com/macros/s/AKfycbx6ntzH26KpNRt4Zzrv2HxCc4okhMjO9d_AHOouuFswfhcT52jlP1ZIetC3l5d8-9xR/exec"
#define SHEET_SENSOR_LOG  "ESP1_SensorLog"
#define SHEET_STATUS_LOG  "ESP1_StatusLog"
#define SHEET_IR_LOG      "ESP1_IRLog"

// ─── Interval ──────────────────────────────────────────────────
#define DATA_READ_INTERVAL_MS   1000UL    // baca sensor tiap 1 detik
#define DATA_SEND_INTERVAL_MS   60000UL   // kirim avg tiap 1 menit

// ─── Fuzzy: Interval komputasi ─────────────────────────────────
#define FUZZY_INTERVAL_MS   10000UL

// ─── AC Auto Off Timer ─────────────────────────────────────────
#define AC_OFF_DELAY_MS         (1UL * 60UL * 1000UL)  // 15 menit

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
#define TOPIC_FUZZY_SETPOINT    "2204129/esp1/fuzzy/setpoint"
#define TOPIC_FUZZY_TEMP        "2204129/esp1/fuzzy/temp"
#define TOPIC_FUZZY_HUMIDITY    "2204129/esp1/fuzzy/humidity"
#define TOPIC_FUZZY_FIRING_LOW  "2204129/esp1/fuzzy/firing/rendah"
#define TOPIC_FUZZY_FIRING_MID  "2204129/esp1/fuzzy/firing/sedang"
#define TOPIC_FUZZY_FIRING_HIGH "2204129/esp1/fuzzy/firing/tinggi"

// ─── SPIFFS ────────────────────────────────────────────────────
#define IR_JSON_PATH  "/irRawCodes.json"