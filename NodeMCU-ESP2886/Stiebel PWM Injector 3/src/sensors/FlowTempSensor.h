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
};

#endif // FLOWTEMPSENSOR_H
