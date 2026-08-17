# Desain Simulasi Fuzzy Python

## Tujuan

Menyediakan simulator Python untuk melakukan reverse-check manual terhadap
algoritma fuzzy Mamdani yang berjalan pada ESP32. Pengguna memasukkan satu
baris data hasil pengujian hardware, kemudian membandingkan output simulator
dengan kolom `Fuzzy_Setpoint` pada spreadsheet.

Simulator tidak membaca Google Sheets atau menggabungkan file log. Proses
perbandingan dengan hardware dilakukan manual sesuai kebutuhan pengujian.

## Ruang Lingkup

Simulator akan dibuat sebagai `tools/simulasi_fuzzy.py` dan mendukung dua mode:

1. Mode interaktif tanpa argumen.
2. Mode command line dengan argumen `--suhu`, `--kelembaban`, `--orang`, dan
   opsional `--hardware-setpoint`.

Nilai suhu dan kelembaban yang dimasukkan dianggap sudah dikalibrasi, seperti
nilai yang disimpan oleh firmware. Simulator tidak menerapkan offset DHT
sekali lagi.

## Arsitektur Perhitungan

Library `scikit-fuzzy` digunakan untuk menghitung membership function:

- `skfuzzy.trapmf` untuk fungsi trapesium.
- `skfuzzy.trimf` untuk fungsi segitiga.

Proses inferensi tidak diserahkan sepenuhnya kepada `skfuzzy.control`.
Evaluasi rule dan agregasi ditulis eksplisit agar dapat dicocokkan dengan
`FuzzyMamdani.cpp`:

1. Fuzzifikasi input suhu, kelembaban, dan hunian.
2. Evaluasi 27 rule dengan operator AND `min`.
3. Pengelompokan firing strength ke output `Rendah`, `Sedang`, dan `Tinggi`
   menggunakan agregasi `max`.
4. Pemotongan membership output menggunakan operator `min`.
5. Agregasi tiga membership output menggunakan operator `max`.
6. Defuzzifikasi centroid pada universe output 16,0 sampai 30,0°C dengan
   interval 0,1°C.
7. Pembatasan nilai crisp ke rentang 16–30°C dan pembulatan ke setpoint
   integer, mengikuti firmware.

Jika seluruh firing strength bernilai nol, simulator mengembalikan nilai
default 23,0°C, sesuai implementasi firmware.

## Parameter Fuzzy

Semua parameter membership function dan rule base akan disalin dari
`include/config.h` versi repo saat simulator dibuat:

- Suhu: `dingin`, `nyaman`, `panas`.
- Kelembaban: `kering`, `normal`, `lembab`.
- Hunian: `sedikit`, `sedang`, `banyak`.
- Output: `rendah`, `sedang`, `tinggi`.
- Rule base: `rules[suhu][kelembaban][hunian]` dengan label 0, 1, dan 2.

Parameter disimpan sebagai konstanta Python yang diberi nama jelas. Simulator
tidak membaca header C++ secara otomatis agar tetap dapat dijalankan sebagai
alat pengujian Python mandiri.

## Antarmuka Pengguna

Contoh mode interaktif:

```text
python tools/simulasi_fuzzy.py
```

Program meminta suhu, kelembaban, jumlah orang, dan secara opsional setpoint
hardware untuk perbandingan.

Contoh mode command line:

```text
python tools/simulasi_fuzzy.py --suhu 27.32 --kelembaban 56.23 --orang 5 --hardware-setpoint 24
```

Output minimal berisi:

- input yang digunakan;
- derajat keanggotaan setiap input;
- firing strength output `Rendah`, `Sedang`, dan `Tinggi`;
- nilai setpoint crisp;
- setpoint integer hasil simulasi;
- status `COCOK` atau `BERBEDA` jika setpoint hardware diberikan.

Format output terminal dibuat berlabel dan mudah disalin ke tabel pengujian
skripsi. Tidak ada ketergantungan pada koneksi internet atau perangkat ESP32
saat simulator dijalankan.

## Dependensi

Dependensi Python yang dibutuhkan:

- `numpy`.
- `scikit-fuzzy`.

Daftar dependensi akan disimpan dalam berkas requirements khusus simulasi agar
tidak mengubah dependensi PlatformIO firmware.

## Verifikasi

Verifikasi simulator mencakup:

- pengujian nilai input di luar atau pada batas membership function;
- pengujian input yang menghasilkan firing strength nol;
- pengujian beberapa kombinasi input yang mewakili data spreadsheet;
- pemeriksaan bahwa hasil yang dibandingkan dengan hardware adalah setpoint
  integer, bukan nilai crisp saja;
- pemeriksaan bahwa input yang telah dikalibrasi tidak menerima koreksi
  tambahan.

Simulator dinyatakan sesuai apabila seluruh tahapan perhitungannya dapat
ditelusuri ke `include/config.h` dan `lib/FuzzyMamdani/FuzzyMamdani.cpp`, serta
hasilnya dapat digunakan untuk membandingkan satu kasus hardware secara
manual.

## Di Luar Ruang Lingkup

- Import otomatis dari Google Sheets atau CSV.
- Pengambilan data langsung dari MQTT.
- Perubahan algoritma fuzzy firmware.
- Pengujian respons fisik AC atau akurasi sensor.
- Penggunaan `skfuzzy.control.ControlSystem` sebagai pengganti inferensi
  eksplisit firmware.
