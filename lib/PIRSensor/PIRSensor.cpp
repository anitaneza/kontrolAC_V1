#include "PIRSensor.h"

PIRSensor::PIRSensor(uint8_t pin)
    : _pin(pin)
{
}

void PIRSensor::begin() {
    pinMode(_pin, INPUT);
    Serial.println("[PIRSensor] Siap.");
}

bool PIRSensor::isDetected() {
    return digitalRead(_pin) == HIGH;
}