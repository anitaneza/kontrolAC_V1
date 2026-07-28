#include "IRTransmitterACController.h"

IRTransmitterACController::IRTransmitterACController(uint8_t txPin, IRRawCodes& rawCodes)
    : _irsend(txPin), _rawCodes(rawCodes)
{
}

void IRTransmitterACController::begin() {
    _irsend.begin();
    Serial.println("[IRTransmitter] Siap.");
}

bool IRTransmitterACController::sendKey(const char* key) {
    uint16_t rawBuf[MAX_RAW_LEN];
    uint16_t rawLen = 0;

    if (!_rawCodes.get(key, rawBuf, rawLen)) {
        Serial.printf("[IRTransmitter] Gagal ambil kode untuk key: %s\n", key);
        return false;
    }

    _irsend.sendRaw(rawBuf, rawLen, 38); // 38kHz carrier
    Serial.printf("[IRTransmitter] Sinyal dikirim untuk key: %s\n", key);
    return true;
}