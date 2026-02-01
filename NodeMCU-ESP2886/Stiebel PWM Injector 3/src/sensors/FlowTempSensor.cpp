#include "FlowTempSensor.h"
#include <Arduino.h>

FlowTempSensor::FlowTempSensor(uint8_t pin)
    : _pin(pin), _oneWire(pin), _sensors(&_oneWire), _lastValue(NAN), _lastRead(0), _maIndex(0), _maCount(0) {
    for (uint8_t i = 0; i < MA_SIZE; i++) _maBuffer[i] = NAN;
}

void FlowTempSensor::begin() {
    pinMode(_pin, INPUT_PULLUP);
    _sensors.begin();
    for (uint8_t i = 0; i < MA_SIZE; i++) _maBuffer[i] = NAN;
    _maIndex = 0;
    _maCount = 0;
}

float FlowTempSensor::read() {
    unsigned long now = millis();
    if (now - _lastRead > 1000 || isnan(_lastValue)) {
        _sensors.requestTemperatures();
        float newVal = _sensors.getTempCByIndex(0);
        _lastRead = now;
        // Only add valid readings to moving average (-127 is error)
        if (newVal != -127.0f) {
            _maBuffer[_maIndex] = newVal;
            _maIndex = (_maIndex + 1) % MA_SIZE;
            if (_maCount < MA_SIZE) _maCount++;
        }
        // Calculate average, ignore NAN and -127
        float sum = 0.0f;
        uint8_t valid = 0;
        for (uint8_t i = 0; i < _maCount; i++) {
            if (!isnan(_maBuffer[i]) && _maBuffer[i] != -127.0f) { sum += _maBuffer[i]; valid++; }
        }
        _lastValue = (valid > 0) ? (sum / valid) : NAN;
    }
    return _lastValue;
}
