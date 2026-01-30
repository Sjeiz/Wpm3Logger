
#include "PwmInSensor.h"
#include <Arduino.h>

static PwmInSensor* _activeInstance = nullptr;

PwmInSensor::PwmInSensor(uint8_t pin)
    : _pin(pin), _lastRise(0), _lastFall(0), _period(0), _highTime(0), _dutyCycle(0), _frequency(0) {}

void PwmInSensor::begin() {
    pinMode(_pin, INPUT);
    _activeInstance = this;
    attachInterruptArg(digitalPinToInterrupt(_pin), isrRise, this, RISING);
    attachInterruptArg(digitalPinToInterrupt(_pin), isrFall, this, FALLING);
}

void IRAM_ATTR PwmInSensor::isrRise(void* arg) {
    PwmInSensor* self = static_cast<PwmInSensor*>(arg);
    uint32_t now = micros();
    self->_period = now - self->_lastRise;
    self->_lastRise = now;
    self->_highTime = 0; // reset high time, will be set on next fall
}

void IRAM_ATTR PwmInSensor::isrFall(void* arg) {
    PwmInSensor* self = static_cast<PwmInSensor*>(arg);
    uint32_t now = micros();
    self->_lastFall = now;
    self->_highTime = now - self->_lastRise;
}

void PwmInSensor::update() {
    // Bereken frequentie en duty cycle op basis van gemeten tijden
    noInterrupts();
    uint32_t period = _period;
    uint32_t highTime = _highTime;
    uint32_t lastRise = _lastRise;
    interrupts();
    uint32_t now = micros();
    const uint32_t timeout = 2000000UL; // 2 seconden zonder puls = always high/low
    if (period > 0 && (now - lastRise) < timeout) {
        _frequency = 1000000.0f / period;
        _dutyCycle = (float)highTime / period * 100.0f;
    } else {
        // Geen pulsen meer: bepaal of altijd hoog of altijd laag
        int pinState = digitalRead(_pin);
        _frequency = 0.0f;
        if (pinState == HIGH) {
            _dutyCycle = 100.0f; // altijd hoog
        } else {
            _dutyCycle = 0.0f; // altijd laag
        }
    }
}

float PwmInSensor::getDutyCycle() const {
    return _dutyCycle;
}

float PwmInSensor::getFrequency() const {
    return _frequency;
}
