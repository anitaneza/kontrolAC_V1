#pragma once

#include <Arduino.h>
#include <IRsend.h>
#include "IRRawCodes.h"

class IRTransmitterACController {
public:
    IRTransmitterACController(uint8_t txPin, IRRawCodes& rawCodes);

    void begin();
    bool sendKey(const char* key);

private:
    IRsend      _irsend;
    IRRawCodes& _rawCodes;
};