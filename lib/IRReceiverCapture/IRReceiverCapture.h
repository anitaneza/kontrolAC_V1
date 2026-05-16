#pragma once

#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>

#define CAPTURE_BUF_SIZE 200

class IRReceiverCapture {
public:
    IRReceiverCapture(uint8_t rxPin);

    void begin();
    void stop();
    bool capture(uint16_t* outBuf, uint16_t& outLen);

private:
    IRrecv  _irrecv;
    decode_results _results;
};