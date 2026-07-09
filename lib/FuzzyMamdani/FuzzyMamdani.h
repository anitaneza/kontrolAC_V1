#pragma once

#include <Arduino.h>

struct TrapMF { float a, b, c, d; };
struct TriMF  { float a, b, c; };

struct TempMF {
    TrapMF dingin;
    TriMF  nyaman;
    TrapMF panas;
};

struct HumidMF {
    TrapMF kering;
    TriMF  normal;
    TrapMF lembab;
};

struct OccupancyMF {
    TrapMF sedikit;
    TriMF  sedang;
    TrapMF banyak;
};

struct SetpointMF {
    TrapMF rendah;
    TriMF  sedang;
    TrapMF tinggi;
};

// rules[suhu][kelembaban][hunian]
struct RuleBase {
    uint8_t rules[3][3][3];
};

struct FuzzyResult {
    float crispSetpoint;
    int   setpointInt;
    float firingRendah;
    float firingSedang;
    float firingTinggi;
};

class FuzzyMamdani {
public:
    FuzzyMamdani(TempMF tempMF, HumidMF humidMF,
                 OccupancyMF occMF, SetpointMF setpointMF,
                 RuleBase ruleBase);

    FuzzyResult compute(float suhu, float kelembaban, float occupancy);

private:
    TempMF      _tempMF;
    HumidMF     _humidMF;
    OccupancyMF _occMF;
    SetpointMF  _setpointMF;
    RuleBase    _ruleBase;

    float trapmf(float x, TrapMF mf);
    float trimf(float x, TriMF mf);
    float defuzzify(float firingRendah, float firingSedang, float firingTinggi);
};