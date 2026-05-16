#include "DHTRoomSensor.h"
#include <Arduino.h>

// Interval minimum baca DHT22 adalah 2 detik
static const unsigned long READ_INTERVAL_MS = 2000;

DHTRoomSensor::DHTRoomSensor(uint8_t pin, uint8_t type)
    : _dht(pin, type), _lastTemp(0.0f), _lastHumid(0.0f), _valid(false)
{
}

void DHTRoomSensor::mulai() {
    _dht.begin();
}

float DHTRoomSensor::bacaTemperature() {
    float t = _dht.readTemperature();
    if (!isnan(t)) {
        _lastTemp = t;
        _valid = true;
    } else {
        _valid = false;
    }
    return _lastTemp;
}

float DHTRoomSensor::bacaHumidity() {
    float h = _dht.readHumidity();
    if (!isnan(h)) {
        _lastHumid = h;
        _valid = true;
    } else {
        _valid = false;
    }
    return _lastHumid;
}

bool DHTRoomSensor::isValid() {
    return _valid;
}