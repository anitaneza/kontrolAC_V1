#include "FuzzyMamdani.h"
#include <algorithm>

FuzzyMamdani::FuzzyMamdani(TempMF tempMF, HumidMF humidMF,
                            SetpointMF setpointMF, RuleBase ruleBase)
    : _tempMF(tempMF), _humidMF(humidMF),
      _setpointMF(setpointMF), _ruleBase(ruleBase)
{
}

// ─── Fungsi Keanggotaan ────────────────────────────────────────

float FuzzyMamdani::trapmf(float x, TrapMF mf) {
    if (x <= mf.a || x >= mf.d) return 0.0f;
    if (x >= mf.b && x <= mf.c) return 1.0f;
    if (x < mf.b) return (x - mf.a) / (mf.b - mf.a);
    return (mf.d - x) / (mf.d - mf.c);
}

float FuzzyMamdani::trimf(float x, TriMF mf) {
    if (x <= mf.a || x >= mf.c) return 0.0f;
    if (x == mf.b)               return 1.0f;
    if (x < mf.b) return (x - mf.a) / (mf.b - mf.a);
    return (mf.c - x) / (mf.c - mf.b);
}

// ─── Defuzzifikasi Centroid ────────────────────────────────────

float FuzzyMamdani::defuzzify(float firingRendah, float firingSedang, float firingTinggi) {
    float numerator   = 0.0f;
    float denominator = 0.0f;
    const float step  = 0.1f;

    for (float x = 16.0f; x <= 30.0f; x += step) {
        // Hitung membership tiap label output lalu clip dengan firing strength
        float muRendah = min(firingRendah, trapmf(x, _setpointMF.rendah));
        float muSedang = min(firingSedang, trimf(x,  _setpointMF.sedang));
        float muTinggi = min(firingTinggi, trapmf(x, _setpointMF.tinggi));

        // Agregasi: ambil max dari semua label
        float muAgregat = max({muRendah, muSedang, muTinggi});

        numerator   += x * muAgregat;
        denominator += muAgregat;
    }

    if (denominator == 0.0f) return 23.0f; // fallback ke tengah range
    return numerator / denominator;
}

// ─── Komputasi Utama ───────────────────────────────────────────

FuzzyResult FuzzyMamdani::compute(float suhu, float kelembaban) {
    // Tahap 1: Fuzzifikasi input suhu
    float muTemp[3];
    muTemp[0] = trapmf(suhu, _tempMF.dingin);
    muTemp[1] = trimf(suhu,  _tempMF.nyaman);
    muTemp[2] = trapmf(suhu, _tempMF.panas);

    // Tahap 2: Fuzzifikasi input kelembaban
    float muHumid[3];
    muHumid[0] = trapmf(kelembaban, _humidMF.rendah);
    muHumid[1] = trimf(kelembaban,  _humidMF.sedang);
    muHumid[2] = trapmf(kelembaban, _humidMF.tinggi);

    // Tahap 3: Evaluasi 9 rule, agregasi per label output
    float firingOut[3] = {0.0f, 0.0f, 0.0f}; // 0=Rendah, 1=Sedang, 2=Tinggi

    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t j = 0; j < 3; j++) {
            float firing = min(muTemp[i], muHumid[j]);
            uint8_t outputLabel = _ruleBase.rules[i][j];
            firingOut[outputLabel] = max(firingOut[outputLabel], firing);
        }
    }

    // Tahap 4: Defuzzifikasi centroid
    float crisp = defuzzify(firingOut[0], firingOut[1], firingOut[2]);
    crisp = constrain(crisp, 16.0f, 30.0f);

    FuzzyResult result;
    result.crispSetpoint = crisp;
    result.setpointInt   = (int)round(crisp);
    result.firingRendah  = firingOut[0];
    result.firingSedang  = firingOut[1];
    result.firingTinggi  = firingOut[2];

    return result;
}