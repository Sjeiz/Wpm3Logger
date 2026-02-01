#ifndef PWMIN_SENSOR_H
#define PWMIN_SENSOR_H

#include <Arduino.h>


class PwmInSensor {
public:
    PwmInSensor(uint8_t pin);
    void begin();
    float read();
    float getFrequency() const;
private:
    uint8_t _pin;
    float _dutyCycle;
    float _frequency;
    uint32_t _lastUpdate;
    // Polling-based sampling members
    uint32_t _sampleStart;
    uint32_t _highTicks;
    uint32_t _totalTicks;
    uint32_t _risingEdges;
    uint8_t _lastPinState;
    // Moving average buffer
    static const uint8_t MA_SIZE = 20;
    float _maBuffer[MA_SIZE];
    uint8_t _maIndex = 0;
    uint8_t _maCount = 0;
};

#endif // PWMIN_SENSOR_H
