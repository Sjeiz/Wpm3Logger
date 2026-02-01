

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
    // Moving average init
    for (uint8_t i = 0; i < MA_SIZE; i++) _maBuffer[i] = 0.0f;
    _maIndex = 0;
    _maCount = 0;
}

float PwmInSensor::read() {
    // Polling-based sampling for PWM input
    const uint32_t samplePeriodMs = 1000; // 1000ms (1s) meetperiode
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
        float newDuty;
        if (_risingEdges == 0) {
            int pinState = digitalRead(_pin);
            if (pinState == HIGH) {
                newDuty = 100.0f;
            } else {
                newDuty = 0.0f;
            }
        } else {
            newDuty = (_totalTicks > 0) ? (100.0f * _highTicks / _totalTicks) : 0.0f;
        }
        // Moving average update
        _maBuffer[_maIndex] = newDuty;
        _maIndex = (_maIndex + 1) % MA_SIZE;
        if (_maCount < MA_SIZE) _maCount++;
        // Bereken gemiddelde
        float sum = 0.0f;
        for (uint8_t i = 0; i < _maCount; i++) sum += _maBuffer[i];
        _dutyCycle = sum / _maCount;
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
