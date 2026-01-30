#ifndef PWMIN_SENSOR_H
#define PWMIN_SENSOR_H

#include <Arduino.h>


class PwmInSensor {
public:
    PwmInSensor(uint8_t pin);
    void begin();
    void update();
    float getDutyCycle() const;
    float getFrequency() const;
private:
    uint8_t _pin;
    volatile uint32_t _lastRise;
    volatile uint32_t _lastFall;
    volatile uint32_t _period;
    volatile uint32_t _highTime;
    volatile float _dutyCycle;
    volatile float _frequency;
    uint32_t _lastUpdate = 0;
    static void IRAM_ATTR isrRise(void* arg);
    static void IRAM_ATTR isrFall(void* arg);
};

#endif // PWMIN_SENSOR_H
