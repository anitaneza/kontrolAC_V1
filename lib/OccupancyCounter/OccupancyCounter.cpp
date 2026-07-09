#include "OccupancyCounter.h"

OccupancyCounter::OccupancyCounter(uint8_t pinFront, uint8_t pinBack, unsigned long timeoutMs)
    : _pinFront(pinFront), _pinBack(pinBack), _timeoutMs(timeoutMs),
      _count(0), _frontTriggered(false), _backTriggered(false),
      _frontTriggerTime(0), _backTriggerTime(0)
{
}

void OccupancyCounter::begin() {
    pinMode(_pinFront, INPUT);
    pinMode(_pinBack,  INPUT);
    Serial.println("[OccupancyCounter] Siap.");
}

void OccupancyCounter::_reset() {
    _frontTriggered   = false;
    _backTriggered    = false;
    _frontTriggerTime = 0;
    _backTriggerTime  = 0;
}

void OccupancyCounter::update() {
    unsigned long now = millis();

    // E18-D80NK output LOW saat mendeteksi objek
    bool frontDetected = digitalRead(_pinFront) == LOW;
    bool backDetected  = digitalRead(_pinBack)  == LOW;

    // Catat waktu trigger pertama kali
    if (frontDetected && !_frontTriggered) {
        _frontTriggered   = true;
        _frontTriggerTime = now;
        Serial.println("[OCC] IR Depan trigger.");
    }

    if (backDetected && !_backTriggered) {
        _backTriggered   = true;
        _backTriggerTime = now;
        Serial.println("[OCC] IR Belakang trigger.");
    }

    // Evaluasi arah setelah kedua sensor pernah trigger
    if (_frontTriggered && _backTriggered) {
        if (_frontTriggerTime <= _backTriggerTime) {
            // Depan dulu → Belakang : masuk
            _count++;
            if (_count < 0) _count = 0;
            Serial.printf("[OCC] Masuk. Count: %d\n", _count);
        } else {
            // Belakang dulu → Depan : keluar
            _count--;
            if (_count < 0) _count = 0;
            Serial.printf("[OCC] Keluar. Count: %d\n", _count);
        }
        _reset();
        return;
    }

    // Timeout: salah satu trigger tapi yang lain tidak dalam waktu timeout
    if (_frontTriggered && !_backTriggered) {
        if (now - _frontTriggerTime >= _timeoutMs) {
            Serial.println("[OCC] Timeout IR Depan, diabaikan.");
            _reset();
        }
    }

    if (_backTriggered && !_frontTriggered) {
        if (now - _backTriggerTime >= _timeoutMs) {
            Serial.println("[OCC] Timeout IR Belakang, diabaikan.");
            _reset();
        }
    }
}

int OccupancyCounter::getCount() {
    return _count;
}