#include "FlowTempSensor.h"
#include <Arduino.h>

FlowTempSensor::FlowTempSensor(uint8_t pin)
    : _pin(pin), _oneWire(pin), _sensors(&_oneWire), _lastValue(NAN), _lastRead(0) {}

void FlowTempSensor::begin() {
    pinMode(_pin, INPUT_PULLUP);
    _sensors.begin();
}

float FlowTempSensor::read() {
    unsigned long now = millis();
    if (now - _lastRead > 1000 || isnan(_lastValue)) {
        _sensors.requestTemperatures();
        _lastValue = _sensors.getTempCByIndex(0);
        _lastRead = now;
    }
    return _lastValue;
}
