#pragma once

#include <Arduino.h>

class OccupancyCounter {
public:
    OccupancyCounter(uint8_t pinFront, uint8_t pinBack, unsigned long timeoutMs);

    void begin();
    void update();
    int  getCount();

private:
    uint8_t       _pinFront;
    uint8_t       _pinBack;
    unsigned long _timeoutMs;
    int           _count;

    // State tracking
    bool          _frontTriggered;
    bool          _backTriggered;
    unsigned long _frontTriggerTime;
    unsigned long _backTriggerTime;

    void _reset();
};