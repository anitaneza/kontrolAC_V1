#pragma once

// ─── DHT Room Sensor ───────────────────────────────────────────
#define DHT_PIN     18
#define DHT_TYPE    DHT22
#define DHT_TEMP_OFFSET   -2.5f
#define DHT_HUMID_OFFSET   4.0f

// ─── IR ────────────────────────────────────────────────────────
#define IR_TX_PIN   4
#define IR_RX_PIN   5
#define IR_RAW_LEN  211

// ─── PIR Sensor ────────────────────────────────────────────────
#define PIR_PIN     27

// ─── Occupancy Counter IR E18-D80NK ────────────────────────────
#define IR_FRONT_PIN            21
#define IR_BACK_PIN             22
#define IR_SENSOR_TIMEOUT_MS    3000UL   // timeout salah satu sensor

// Trapesium kiri  : [a, b, c, d]
// Segitiga        : [a, b, c]
// Trapesium kanan : [a, b, c, d]

// ─── Fuzzy Mamdani: Membership Function Hunian (Orang) ─────────
#define OCC_SEDIKIT_A   0.0f
#define OCC_SEDIKIT_B   1.0f
#define OCC_SEDIKIT_C   3.0f
#define OCC_SEDIKIT_D   4.0f

#define OCC_SEDANG_A    3.0f
#define OCC_SEDANG_B    4.0f
#define OCC_SEDANG_C    7.0f

#define OCC_BANYAK_A    5.0f
#define OCC_BANYAK_B    8.0f
#define OCC_BANYAK_C    9.0f
#define OCC_BANYAK_D    10.0f

// ─── Fuzzy Mamdani: Membership Function Suhu (°C) ──────────────
#define TEMP_DINGIN_A   16.0f
#define TEMP_DINGIN_B   18.0f
#define TEMP_DINGIN_C   22.0f
#define TEMP_DINGIN_D   24.0f

#define TEMP_NYAMAN_A   23.0f
#define TEMP_NYAMAN_B   25.0f
#define TEMP_NYAMAN_C   27.0f

#define TEMP_PANAS_A    27.0f
#define TEMP_PANAS_B    28.0f
#define TEMP_PANAS_C    30.0f
#define TEMP_PANAS_D    35.0f

// ─── Fuzzy Mamdani: Membership Function Kelembaban (%) ─────────
#define HUMID_KERING_A  20.0f
#define HUMID_KERING_B  30.0f
#define HUMID_KERING_C  50.0f
#define HUMID_KERING_D  55.0f

#define HUMID_NORMAL_A  50.0f
#define HUMID_NORMAL_B  60.0f
#define HUMID_NORMAL_C  70.0f

#define HUMID_LEMBAB_A  60.0f
#define HUMID_LEMBAB_B  75.0f
#define HUMID_LEMBAB_C  90.0f
#define HUMID_LEMBAB_D  100.0f

// ─── Fuzzy Mamdani: Membership Function Output Setpoint (°C) ───
#define SETPOINT_RENDAH_A   17.0f
#define SETPOINT_RENDAH_B   18.0f
#define SETPOINT_RENDAH_C   22.0f
#define SETPOINT_RENDAH_D   24.0f

#define SETPOINT_SEDANG_A   23.0f
#define SETPOINT_SEDANG_B   25.0f
#define SETPOINT_SEDANG_C   30.0f

#define SETPOINT_TINGGI_A   25.0f
#define SETPOINT_TINGGI_B   30.0f
#define SETPOINT_TINGGI_C   32.0f
#define SETPOINT_TINGGI_D   33.0f

// ─── Fuzzy Mamdani: Rule Base ──────────────────────────────────
// rules[suhu][kelembaban][hunian] → 0=Rendah, 1=Sedang, 2=Tinggi

// SUHU = DINGIN
#define RULE_000  2
#define RULE_001  2
#define RULE_002  1
#define RULE_010  2
#define RULE_011  2
#define RULE_012  1
#define RULE_020  1
#define RULE_021  1
#define RULE_022  0

// SUHU = NYAMAN
#define RULE_100  2
#define RULE_101  2
#define RULE_102  1
#define RULE_110  1
#define RULE_111  1
#define RULE_112  0
#define RULE_120  0
#define RULE_121  0
#define RULE_122  0

// SUHU = PANAS
#define RULE_200  1
#define RULE_201  1
#define RULE_202  0
#define RULE_210  0
#define RULE_211  0
#define RULE_212  0
#define RULE_220  0
#define RULE_221  0
#define RULE_222  0


// ─── Interval ──────────────────────────────────────────────────
#define DATA_READ_INTERVAL_MS   1000UL    // baca sensor tiap 1 detik
#define DATA_SEND_INTERVAL_MS   60000UL   // kirim avg tiap 1 menit

// ─── Fuzzy: Interval komputasi ─────────────────────────────────
#define FUZZY_INTERVAL_MS   (5UL * 60UL * 1000UL)

// ─── AC Auto Off Timer ─────────────────────────────────────────
#define AC_OFF_DELAY_MS         (10UL * 60UL * 1000UL)  // 15 menit

// ─── WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID   "sppg kamal"
#define WIFI_PASS   "BGN#38A1225"

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
#define TOPIC_AC_MODE            "2204129/esp1/ac/mode"
#define TOPIC_AC_STATUS         "2204129/esp1/ac/status"
#define TOPIC_FUZZY_SETPOINT    "2204129/esp1/fuzzy/setpoint"
#define TOPIC_FUZZY_TEMP        "2204129/esp1/fuzzy/temp"
#define TOPIC_FUZZY_HUMIDITY    "2204129/esp1/fuzzy/humidity"
#define TOPIC_FUZZY_FIRING_LOW  "2204129/esp1/fuzzy/firing/rendah"
#define TOPIC_FUZZY_FIRING_MID  "2204129/esp1/fuzzy/firing/sedang"
#define TOPIC_FUZZY_FIRING_HIGH "2204129/esp1/fuzzy/firing/tinggi"
#define TOPIC_AC_POWER         "2204129/esp1/ac/power"
#define TOPIC_AC_SETPOINT      "2204129/esp1/ac/setpoint"

// ─── Apps Script ───────────────────────────────────────────────
#define APPS_SCRIPT_URL   "https://script.google.com/macros/s/AKfycbx6ntzH26KpNRt4Zzrv2HxCc4okhMjO9d_AHOouuFswfhcT52jlP1ZIetC3l5d8-9xR/exec"
#define SHEET_SENSOR_LOG  "ESP1_SensorLog"
#define SHEET_STATUS_LOG  "ESP1_StatusLog"
#define SHEET_IR_LOG      "ESP1_IRLog"

// ─── SPIFFS ────────────────────────────────────────────────────
#define IR_JSON_PATH  "/irRawCodes.json" 