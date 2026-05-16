#pragma once

#include <Arduino.h>

// ─── Membership Function Parameters ────────────────────────────
// Suhu (°C): [a, b, c, d] untuk trapesium, [a, b, c] untuk segitiga
struct TrapMF  { float a, b, c, d; };
struct TriMF   { float a, b, c; };

// Input suhu
struct TempMF {
    TrapMF dingin;  // trapesium kiri
    TriMF  nyaman;  // segitiga
    TrapMF panas;   // trapesium kanan
};

// Input kelembaban
struct HumidMF {
    TrapMF rendah;  // trapesium kiri
    TriMF  sedang;  // segitiga
    TrapMF tinggi;  // trapesium kanan
};

// Output setpoint
struct SetpointMF {
    TrapMF rendah;  // trapesium kiri
    TriMF  sedang;  // segitiga
    TrapMF tinggi;  // trapesium kanan
};

// ─── Rule Base ─────────────────────────────────────────────────
// 0 = Rendah, 1 = Sedang, 2 = Tinggi
// rules[suhu][kelembaban] = output label
struct RuleBase {
    uint8_t rules[3][3];
};

// ─── Hasil Fuzzy ───────────────────────────────────────────────
struct FuzzyResult {
    float  crispSetpoint;   // nilai crisp hasil defuzzifikasi
    int    setpointInt;     // dibulatkan ke integer, siap kirim ke AC
    float  firingRendah;    // firing strength output Rendah
    float  firingSedang;    // firing strength output Sedang
    float  firingTinggi;    // firing strength output Tinggi
};

// ─── Class ─────────────────────────────────────────────────────
class FuzzyMamdani {
public:
    FuzzyMamdani(TempMF tempMF, HumidMF humidMF, SetpointMF setpointMF, RuleBase ruleBase);

    FuzzyResult compute(float suhu, float kelembaban);

private:
    TempMF     _tempMF;
    HumidMF    _humidMF;
    SetpointMF _setpointMF;
    RuleBase   _ruleBase;

    float trapmf(float x, TrapMF mf);
    float trimf(float x, TriMF mf);
    float defuzzify(float firingRendah, float firingSedang, float firingTinggi);
};