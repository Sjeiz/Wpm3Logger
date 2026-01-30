

#include "PwmInSensor.h"
#include <Arduino.h>

PwmInSensor::PwmInSensor(uint8_t pin)
    : _pin(pin), _dutyCycle(0), _frequency(0), _lastUpdate(0), _sampleStart(0), _highTicks(0), _totalTicks(0), _risingEdges(0), _lastPinState(LOW) {}

void PwmInSensor::begin() {
    pinMode(_pin, INPUT_PULLUP);
    _lastUpdate = millis();
    _sampleStart = millis();
    _highTicks = 0;
    _totalTicks = 0;
    _risingEdges = 0;
    _lastPinState = digitalRead(_pin);
}

float PwmInSensor::read() {
    // Polling-based sampling for PWM input
    const uint32_t samplePeriodMs = 100; // 100ms meetperiode
    uint8_t pinState = digitalRead(_pin);
    if (pinState == HIGH) _highTicks++;
    _totalTicks++;
    // Detect rising edge
    if (_lastPinState == LOW && pinState == HIGH) {
        _risingEdges++;
    }
    _lastPinState = pinState;

    uint32_t now = millis();
    if (now - _sampleStart >= samplePeriodMs) {
        // Bereken frequentie
        _frequency = (_risingEdges * 1000.0f) / (now - _sampleStart);
        // Edge case: geen flanken in meetperiode
        if (_risingEdges == 0) {
            int pinState = digitalRead(_pin);
            if (pinState == HIGH) {
                _dutyCycle = 100.0f;
            } else {
                _dutyCycle = 0.0f;
            }
        } else {
            _dutyCycle = (_totalTicks > 0) ? (100.0f * _highTicks / _totalTicks) : 0.0f;
        }
        // Reset counters voor volgende periode
        _sampleStart = now;
        _highTicks = 0;
        _totalTicks = 0;
        _risingEdges = 0;
    }

    return _dutyCycle;
}

float PwmInSensor::getFrequency() const {
    return _frequency;
}
