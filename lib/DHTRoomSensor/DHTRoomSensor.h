#pragma once

#include <DHT.h>

class DHTRoomSensor {
public:
    DHTRoomSensor(uint8_t pin, uint8_t type);

    void  begin();
    float readTemperature();  // dalam Celsius
    float readHumidity();     // dalam %RH
    bool  isValid();          // true jika bacaan terakhir tidak NaN

private:
    DHT    _dht;
    float  _lastTemp;
    float  _lastHumid;
    bool   _valid;
};