#pragma once

#include <Arduino.h>

class PIRSensor {
public:
    PIRSensor(uint8_t pin);

    void begin();
    bool isDetected();

private:
    uint8_t _pin;
};