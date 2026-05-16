#include "IRReceiverCapture.h"

IRReceiverCapture::IRReceiverCapture(uint8_t rxPin)
    : _irrecv(rxPin, CAPTURE_BUF_SIZE)
{
}

void IRReceiverCapture::begin() {
    _irrecv.enableIRIn();
    Serial.println("[IRReceiver] Aktif, menunggu sinyal...");
}

void IRReceiverCapture::stop() {
    _irrecv.disableIRIn();
    Serial.println("[IRReceiver] Nonaktif.");
}

bool IRReceiverCapture::capture(uint16_t* outBuf, uint16_t& outLen) {
    if (!_irrecv.decode(&_results)) return false;

    outLen = 0;
    if (_results.rawlen > 1) {
        for (uint16_t i = 1; i < _results.rawlen && i < CAPTURE_BUF_SIZE; i++) {
            outBuf[outLen++] = _results.rawbuf[i] * RAWTICK;
        }
    }

    _irrecv.resume();
    return outLen > 0;
}