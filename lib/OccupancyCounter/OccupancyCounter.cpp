// ============================================================
// KODE LAMA — di-comment untuk referensi
// ============================================================
/*
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

void OccupancyCounter::setCount(int count) {
    _count = max(0, count);
    _resetSeq();
    _cooldownUntil = millis() + OCC_COOLDOWN_MS;
    Serial.printf("[OCC] Count diatur manual: %d\n", _count);
}
*/
/* // ============================================================
// KODE LAMA V2 — overlap-based dengan debounce + cooldown (DIKOMMENT)
// ============================================================
#include "OccupancyCounter.h"

#define OCC_DEBOUNCE_MS       40
#define OCC_CLEAR_DEBOUNCE_MS 15
#define OCC_MIN_OVERLAP_MS    30
#define OCC_COOLDOWN_MS       1500
#define OCC_MIN_BLOCK_MS      150

OccupancyCounter::OccupancyCounter(uint8_t pinFront, uint8_t pinBack, unsigned long timeoutMs)
    : _pinFront(pinFront), _pinBack(pinBack), _timeoutMs(timeoutMs),
      _count(0),
      _frontState(false), _backState(false),
      _frontDebounceTime(0), _backDebounceTime(0),
      _frontBlocked(false), _backBlocked(false),
      _frontBlockSince(0), _backBlockSince(0),
      _seqActive(false), _seqIsFrontFirst(false), _seqStartTime(0),
      _overlapSeen(false), _countReady(false), _countPendingEnter(false),
      _lastResetTime(0),
      _cooldownUntil(0)
{}

void OccupancyCounter::begin() {
    pinMode(_pinFront, INPUT);
    pinMode(_pinBack,  INPUT);
    Serial.println("[OccupancyCounter] Siap (overlap-based).");
}

void OccupancyCounter::_debounceSensor(bool raw, bool &state, unsigned long &debounceTime,
                                       bool &blocked, unsigned long &blockSince, unsigned long now) {
    if (raw == state) {
        debounceTime = now;
        return;
    }
    if (now - debounceTime >= OCC_DEBOUNCE_MS) {
        state = raw;
        debounceTime = now;
        if (raw) {
            blocked = true;
            blockSince = now;
        } else {
            blocked = false;
        }
    }
}

void OccupancyCounter::_resetSeq() {
    _seqActive = false;
    _seqIsFrontFirst = false;
    _seqStartTime = 0;
    _overlapSeen = false;
    _countReady = false;
    _countPendingEnter = false;
    _lastResetTime = millis();
}

void OccupancyCounter::update() {
    unsigned long now = millis();

    // ── 1. Baca raw + debounce ──
    bool frontRaw = digitalRead(_pinFront) == LOW;
    bool backRaw  = digitalRead(_pinBack)  == LOW;

    _debounceSensor(frontRaw, _frontState, _frontDebounceTime,
                    _frontBlocked, _frontBlockSince, now);
    _debounceSensor(backRaw,  _backState,  _backDebounceTime,
                    _backBlocked,  _backBlockSince,  now);

    // ── 2. Cooldown ──
    if (now < _cooldownUntil) return;

    // ── 3. Deteksi overlap (kedua sensor blocked simultan) ──
    if (_frontBlocked && _backBlocked) {
        if (!_overlapSeen) {
            _overlapSeen = true;
            Serial.println("[OCC] Overlap terdeteksi.");
        }
    }

    // ── 4. Sequence state machine ──
    if (!_seqActive && !_countReady && (now - _lastResetTime > 100)) {
        if (_frontBlocked && !_backBlocked) {
            _seqActive = true;
            _seqIsFrontFirst = true;
            _seqStartTime = now;
            Serial.println("[OCC] Seq mulai: Depan duluan.");
        } else if (_backBlocked && !_frontBlocked) {
            _seqActive = true;
            _seqIsFrontFirst = false;
            _seqStartTime = now;
            Serial.println("[OCC] Seq mulai: Belakang duluan.");
        }
    }

    // ── 5. Cek invalid: sensor pertama clear sebelum overlap ──
    if (_seqActive && !_overlapSeen) {
        bool firstLost = _seqIsFrontFirst ? !_frontBlocked : !_backBlocked;
        if (firstLost) {
            Serial.println("[OCC] Seq invalid: sensor pertama clear duluan.");
            _resetSeq();
        }
    }

    // ── 6. Overlap tercapai → validasi durasi ──
    if (_seqActive && _overlapSeen && !_countReady) {
        unsigned long refDuration = _seqIsFrontFirst ?
            (now - _frontBlockSince) : (now - _backBlockSince);
        if (refDuration >= OCC_MIN_BLOCK_MS) {
            _countReady = true;
            _countPendingEnter = _seqIsFrontFirst;
            Serial.printf("[OCC] Overlap valid (%lums).\n", refDuration);
        }
    }

    // ── 7. Finalisasi count setelah kedua sensor clear ──
    if (_countReady && !_frontBlocked && !_backBlocked) {
        if (_countPendingEnter) {
            _count++;
            if (_count < 0) _count = 0;
            Serial.printf("[OCC] >> Masuk. Count: %d\n", _count);
        } else {
            _count--;
            if (_count < 0) _count = 0;
            Serial.printf("[OCC] >> Keluar. Count: %d\n", _count);
        }
        _cooldownUntil = now + OCC_COOLDOWN_MS;
        Serial.printf("[OCC] Cooldown %dms.\n", OCC_COOLDOWN_MS);
        _resetSeq();
    }

    // ── 8. Timeout: partial trigger ──
    if (_seqActive && !_overlapSeen) {
        if (now - _seqStartTime >= _timeoutMs) {
            Serial.println("[OCC] Timeout sequence.");
            _resetSeq();
        }
    }
}

int OccupancyCounter::getCount() {
    return _count;
}

void OccupancyCounter::setCount(int count) {
    _count = max(0, count);
    _resetSeq();
    _cooldownUntil = millis() + OCC_COOLDOWN_MS;
    Serial.printf("[OCC] Count diatur manual: %d\n", _count);
}
*/

// ============================================================
// KODE BARU V3 — first clear + cocok niat-hasil + event timeout
// ============================================================
#include "OccupancyCounter.h"

#define OCC_DEBOUNCE_MS       40
#define OCC_COOLDOWN_MS       1500
#define OCC_MIN_BLOCK_MS      150
#define OCC_PARTIAL_CLEAR_MS  150
#define OCC_MAX_EVENT_MS      5000

OccupancyCounter::OccupancyCounter(uint8_t pinFront, uint8_t pinBack, unsigned long timeoutMs)
    : _pinFront(pinFront), _pinBack(pinBack), _timeoutMs(timeoutMs),
      _count(0),
      _frontState(false), _backState(false),
      _frontDebounceTime(0), _backDebounceTime(0),
      _frontBlocked(false), _backBlocked(false),
      _frontBlockSince(0), _backBlockSince(0),
      _seqActive(false), _seqIsFrontFirst(false), _seqStartTime(0),
      _overlapSeen(false), _countReady(false), _countPendingEnter(false),
      _lastResetTime(0),
      _cooldownUntil(0),
      _eventStartTime(0),
      _firstClearCaptured(false), _firstClearIsFront(false),
      _partialClearStartTime(0), _partialIsFrontClear(false)
{}

void OccupancyCounter::begin() {
    pinMode(_pinFront, INPUT);
    pinMode(_pinBack,  INPUT);
    Serial.println("[OccupancyCounter] Siap (v3: first-clear + event-timeout).");
}

void OccupancyCounter::_debounceSensor(bool raw, bool &state, unsigned long &debounceTime,
                                       bool &blocked, unsigned long &blockSince, unsigned long now) {
    if (raw == state) {
        debounceTime = now;
        return;
    }
    if (now - debounceTime >= OCC_DEBOUNCE_MS) {
        state = raw;
        debounceTime = now;
        if (raw) {
            blocked = true;
            blockSince = now;
        } else {
            blocked = false;
        }
    }
}

void OccupancyCounter::_resetSeq() {
    _seqActive = false;
    _seqIsFrontFirst = false;
    _seqStartTime = 0;
    _overlapSeen = false;
    _countReady = false;
    _countPendingEnter = false;
    _lastResetTime = millis();
    _eventStartTime = 0;
    _firstClearCaptured = false;
    _firstClearIsFront = false;
    _partialClearStartTime = 0;
    _partialIsFrontClear = false;
}

void OccupancyCounter::update() {
    unsigned long now = millis();

    // ── 1. Baca raw + debounce ──
    bool frontRaw = digitalRead(_pinFront) == LOW;
    bool backRaw  = digitalRead(_pinBack)  == LOW;

    _debounceSensor(frontRaw, _frontState, _frontDebounceTime,
                    _frontBlocked, _frontBlockSince, now);
    _debounceSensor(backRaw,  _backState,  _backDebounceTime,
                    _backBlocked,  _backBlockSince,  now);

    // ── 2. Cooldown ──
    if (now < _cooldownUntil) return;

    // ── 3. Sequence state machine ──
    if (!_seqActive && (now - _lastResetTime > 100)) {
        if (_frontBlocked && !_backBlocked) {
            _seqActive = true;
            _seqIsFrontFirst = true;
            _seqStartTime = now;
            _eventStartTime = now;
            Serial.println("[OCC] Seq mulai: Depan duluan.");
        } else if (_backBlocked && !_frontBlocked) {
            _seqActive = true;
            _seqIsFrontFirst = false;
            _seqStartTime = now;
            _eventStartTime = now;
            Serial.println("[OCC] Seq mulai: Belakang duluan.");
        }
    }

    // ── 4. Cek invalid: sensor pertama clear sebelum overlap ──
    if (_seqActive && !_overlapSeen) {
        bool firstLost = _seqIsFrontFirst ? !_frontBlocked : !_backBlocked;
        if (firstLost) {
            Serial.println("[OCC] Seq invalid: sensor pertama clear duluan.");
            _resetSeq();
        }
    }

    // ── 5. Deteksi overlap ──
    if (_seqActive && _frontBlocked && _backBlocked) {
        if (!_overlapSeen) {
            _overlapSeen = true;
            Serial.println("[OCC] Overlap.");
        }
        if (!_countReady) {
            unsigned long refDuration = _seqIsFrontFirst ?
                (now - _backBlockSince) : (now - _frontBlockSince);
            if (refDuration >= OCC_MIN_BLOCK_MS) {
                _countReady = true;
                Serial.printf("[OCC] Overlap valid (%lums).\n", refDuration);
            }
        }
    }

    // ── 6. Setelah countReady: cari partial clear stabil untuk arah ──
    if (_countReady) {
        bool frontClear = !_frontBlocked;
        bool backClear  = !_backBlocked;

        if (frontClear && !backClear) {
            // Depan clear, belakang masih block → potensi KELUAR
            if (_partialClearStartTime == 0 || !_partialIsFrontClear) {
                _partialClearStartTime = now;
                _partialIsFrontClear = true;
            } else if (now - _partialClearStartTime >= OCC_PARTIAL_CLEAR_MS) {
                _firstClearCaptured = true;
                _firstClearIsFront = true;
                _partialClearStartTime = now;
            }
        } else if (!frontClear && backClear) {
            // Belakang clear, depan masih block → potensi MASUK
            if (_partialClearStartTime == 0 || _partialIsFrontClear) {
                _partialClearStartTime = now;
                _partialIsFrontClear = false;
            } else if (now - _partialClearStartTime >= OCC_PARTIAL_CLEAR_MS) {
                _firstClearCaptured = true;
                _firstClearIsFront = false;
                _partialClearStartTime = now;
            }
        } else {
            // Kedua clear atau kedua block → reset partial
            _partialClearStartTime = 0;
        }
    }

    // ── 7. Finalisasi count ──
    if (_countReady && !_frontBlocked && !_backBlocked) {
        unsigned long eventDuration = now - _eventStartTime;

        if (eventDuration <= OCC_MAX_EVENT_MS && _firstClearCaptured) {
            bool intentExit = !_seqIsFrontFirst;
            bool resultExit = _firstClearIsFront;

            if (intentExit == resultExit) {
                if (resultExit) {
                    _count--;
                    if (_count < 0) _count = 0;
                    Serial.printf("[OCC] >> Keluar. Count: %d\n", _count);
                } else {
                    _count++;
                    if (_count < 0) _count = 0;
                    Serial.printf("[OCC] >> Masuk. Count: %d\n", _count);
                }
            } else {
                Serial.println("[OCC] Niat-hasil tidak cocok, diabaikan.");
            }
        } else {
            Serial.printf("[OCC] Event diabaikan (durasi=%lums, arah=%d).\n",
                          eventDuration, _firstClearCaptured);
        }

        _cooldownUntil = now + OCC_COOLDOWN_MS;
        Serial.printf("[OCC] Cooldown %dms.\n", OCC_COOLDOWN_MS);
        _resetSeq();
    }

    // ── 8. Timeout: partial trigger tanpa overlap ──
    if (_seqActive && !_overlapSeen) {
        if (now - _seqStartTime >= _timeoutMs) {
            Serial.println("[OCC] Timeout sequence.");
            _resetSeq();
        }
    }

    // ── 9. Event timeout: countReady tapi tak kunjung dapat arah ──
    if (_countReady && !_firstClearCaptured) {
        if (now - _eventStartTime >= OCC_MAX_EVENT_MS) {
            Serial.println("[OCC] Event timeout (max durasi), diabaikan.");
            _cooldownUntil = now + OCC_COOLDOWN_MS;
            _resetSeq();
        }
    }
}

int OccupancyCounter::getCount() {
    return _count;
}

void OccupancyCounter::setCount(int count) {
    _count = max(0, count);
    _resetSeq();
    _cooldownUntil = millis() + OCC_COOLDOWN_MS;
    Serial.printf("[OCC] Count diatur manual: %d\n", _count);
}