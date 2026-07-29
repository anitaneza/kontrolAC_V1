#pragma once

#include <Arduino.h>

class OccupancyCounter {
public:
    OccupancyCounter(uint8_t pinFront, uint8_t pinBack, unsigned long timeoutMs);

    void begin();
    void update();
    int  getCount();
    void setCount(int count);

private:
    uint8_t       _pinFront;
    uint8_t       _pinBack;
    unsigned long _timeoutMs;
    int           _count;

    // ═══════════ KODE LAMA (di-comment) ═══════════
    // bool          _frontTriggered;
    // bool          _backTriggered;
    // unsigned long _frontTriggerTime;
    // unsigned long _backTriggerTime;
    // void _reset();

    // ═══════════ KODE BARU ═══════════
    bool          _frontState;
    bool          _backState;
    unsigned long _frontDebounceTime;
    unsigned long _backDebounceTime;
    bool          _frontBlocked;
    bool          _backBlocked;
    unsigned long _frontBlockSince;
    unsigned long _backBlockSince;

    bool          _seqActive;
    bool          _seqIsFrontFirst;
    unsigned long _seqStartTime;
    bool          _overlapSeen;
    bool          _countReady;
    bool          _countPendingEnter;
    unsigned long _lastResetTime;

    unsigned long _cooldownUntil;

    // ═══════════ KODE BARU V3 (first clear + cocok niat-hasil + event timeout) ═══════════
    unsigned long _eventStartTime;
    bool          _firstClearCaptured;
    bool          _firstClearIsFront;
    unsigned long _partialClearStartTime;
    bool          _partialIsFrontClear;

    void _resetSeq();
    void _debounceSensor(bool raw, bool &state, unsigned long &debounceTime,
                         bool &blocked, unsigned long &blockSince, unsigned long now);
};