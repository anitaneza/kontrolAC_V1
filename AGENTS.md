# AGENTS.md — kontrolAC_V1

Proyek embedded berbasis **PlatformIO** untuk **ESP32 DOIT DevKit v1**. Sistem kendali AC ruangan dengan sensor DHT22 + PIR, logika fuzzy Mamdani, transmisi IR, dan logging ke MQTT + Google Apps Script.

## Identitas Proyek

- **Board target**: `esp32doit-devkit-v1` (`platformio.ini`)
- **Framework**: Arduino
- **Filesystem**: SPIFFS (`board_build.filesystem = spiffs`)
- **Baud serial / monitor**: 115200
- **Entry point**: `src/main.cpp`
- **Konfigurasi utama**: `include/config.h`

## Arsitektur

```
src/main.cpp
├── lib/DHTRoomSensor        → baca DHT22
├── lib/PIRSensor            → deteksi okupasi
├── lib/WifiHandler          → koneksi WiFi
├── lib/MQTTHandler          → PubSubClient wrapper
├── lib/SPIFFSHandler        → baca/tulis SPIFFS
├── lib/IRRawCodes           → load/update kode IR dari JSON SPIFFS
├── lib/IRTransmitterACController → kirim IR raw
├── lib/IRReceiverCapture    → capture IR raw (mode config)
├── lib/FuzzyMamdani         → inferensi setpoint AC
└── lib/HTTPLogger           → POST ke Google Apps Script
```

- `lib/SHTRoomSensor/` ada tapi **kosong**; tidak digunakan.
- `data/irRawCodes.json` adalah seed file kode IR; saat runtime firmware memakai salinan di SPIFFS (`/irRawCodes.json`).

## Pinout (lihat `include/config.h`)

| Fungsi | Pin |
|--------|-----|
| DHT22  | GPIO 18 |
| IR TX  | GPIO 4  |
| IR RX  | GPIO 5  |
| PIR    | GPIO 21 |

## State Machine

`main.cpp` mengelola state:

- `NORMAL` — operasi normal (baca sensor, fuzzy, kontrol AC)
- `CONFIG` — mode konfigurasi IR (tunggu perintah capture)
- `CAPTURING` — sedang merekam sinyal IR
- `WAITING_CONFIRM` — menunggu konfirmasi topic `…/capture/confirm`

Perintah MQTT untuk alur capture:

1. Publish `2204129/esp1/mode` = `config`
2. Publish `2204129/esp1/capture` = `<key>` (mis. `21`)
3. MCU publish hasil ke `2204129/esp1/capture/result`
4. Publish `2204129/esp1/capture/confirm` = `<key>` untuk menyimpan ke SPIFFS

## Build & Upload

```bash
# Build
pio run

# Upload firmware
pio run --target upload

# Upload filesystem (data/irRawCodes.json ke SPIFFS)
pio run --target uploadfs

# Serial monitor
pio device monitor --baud 115200
```

> Pastikan `data/irRawCodes.json` sudah di-upload ke SPIFFS saat pertama kali flash atau setelah mengubah kode IR.

## Dependensi

Dikelola PlatformIO (`platformio.ini`):

- `adafruit/DHT sensor library@^1.4.7`
- `adafruit/Adafruit Unified Sensor@^1.1.15`
- `knolleary/PubSubClient@^2.8`
- `bblanchon/ArduinoJson@^7.2.2`
- `crankyoldgit/IRremoteESP8266@^2.9.0`
- `adafruit/Adafruit SHT31 Library@^2.2.2` (terpasang, belum dipakai)
- `adafruit/Adafruit BusIO@^1.17.4`

## Aturan Coding

- Library lokal diletakkan di `lib/<NamaLibrary>/` dengan header dan implementasi.
- Header project dari `include/` di-include dengan tanda petik, e.g. `#include "config.h"`.
- Pin, membership function fuzzy, rule base, topic MQTT, dan credential WiFi/MQTT di-#define di `include/config.h`.
- Semua log serial pakai bahasa Indonesia dan awali dengan tag `[Komponen]`.

## Catatan Operasional

- **Kalibrasi DHT**: `DHT_TEMP_OFFSET = -3.0f`, `DHT_HUMID_OFFSET = +2.0f` (lihat `config.h`).
- **Interval**: baca sensor setiap 1 detik; kirim rata-rata setiap 1 menit; fuzzy setiap 10 detik.
- **AC off delay**: 15 menit setelah PIR tidak mendeteksi orang (`AC_OFF_DELAY_MS`).
- Kode IR suhu (`16`–`30`) serta `on`/`off` disimpan di SPIFFS; `data/irRawCodes.json` hanya seed awal.
- `HTTPLogger` mengirim sensor log, status log, dan IR log ke Apps Script endpoint yang hardcoded di `config.h`.
