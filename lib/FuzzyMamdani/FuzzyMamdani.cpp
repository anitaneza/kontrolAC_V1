#include "FuzzyMamdani.h"
#include <algorithm>

FuzzyMamdani::FuzzyMamdani(TempMF tempMF, HumidMF humidMF,
                            OccupancyMF occMF, SetpointMF setpointMF,
                            RuleBase ruleBase)
    : _tempMF(tempMF), _humidMF(humidMF), _occMF(occMF),
      _setpointMF(setpointMF), _ruleBase(ruleBase)
{
}

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

float FuzzyMamdani::defuzzify(float firingRendah, float firingSedang, float firingTinggi) {
    float numerator   = 0.0f;
    float denominator = 0.0f;
    const float step  = 0.1f;

    for (float x = 16.0f; x <= 30.0f; x += step) {
        float muRendah  = min(firingRendah, trapmf(x, _setpointMF.rendah));
        float muSedang  = min(firingSedang, trimf(x,  _setpointMF.sedang));
        float muTinggi  = min(firingTinggi, trapmf(x, _setpointMF.tinggi));
        float muAgregat = max({muRendah, muSedang, muTinggi});

        numerator   += x * muAgregat;
        denominator += muAgregat;
    }

    if (denominator == 0.0f) return 23.0f;
    return numerator / denominator;
}

FuzzyResult FuzzyMamdani::compute(float suhu, float kelembaban, float occupancy) {
    // Fuzzifikasi suhu
    float muTemp[3];
    muTemp[0] = trapmf(suhu, _tempMF.dingin);
    muTemp[1] = trimf(suhu,  _tempMF.nyaman);
    muTemp[2] = trapmf(suhu, _tempMF.panas);

    // Fuzzifikasi kelembaban
    float muHumid[3];
    muHumid[0] = trapmf(kelembaban, _humidMF.kering);
    muHumid[1] = trimf(kelembaban,  _humidMF.normal);
    muHumid[2] = trapmf(kelembaban, _humidMF.lembab);

    // Fuzzifikasi hunian
    float muOcc[3];
    muOcc[0] = trapmf(occupancy, _occMF.sedikit);
    muOcc[1] = trimf(occupancy,  _occMF.sedang);
    muOcc[2] = trapmf(occupancy, _occMF.banyak);

    // Evaluasi 27 rule
    float firingOut[3] = {0.0f, 0.0f, 0.0f};

    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t j = 0; j < 3; j++) {
            for (uint8_t k = 0; k < 3; k++) {
                float firing     = min({muTemp[i], muHumid[j], muOcc[k]});
                uint8_t outLabel = _ruleBase.rules[i][j][k];
                firingOut[outLabel] = max(firingOut[outLabel], firing);
            }
        }
    }

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