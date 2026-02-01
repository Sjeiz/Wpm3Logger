#ifndef FLOWTEMPSENSOR_H
#define FLOWTEMPSENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

class FlowTempSensor {
public:
    FlowTempSensor(uint8_t pin);
    void begin();
    float read();
private:
    uint8_t _pin;
    OneWire _oneWire;
    DallasTemperature _sensors;
    float _lastValue;
    unsigned long _lastRead;
    // Moving average buffer
    static const uint8_t MA_SIZE = 5;
    float _maBuffer[MA_SIZE];
    uint8_t _maIndex = 0;
    uint8_t _maCount = 0;
};

#endif // FLOWTEMPSENSOR_H
