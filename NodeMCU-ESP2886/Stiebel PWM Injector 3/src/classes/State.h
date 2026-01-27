#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <Arduino.h>

class State {
public:
    virtual ~State() {}
    virtual void enter() = 0;
    virtual void handle() = 0;
    virtual void exit() = 0;
    virtual const char* name() const = 0;
    virtual State* transition(uint16_t modbusStatus) = 0;
};

#endif // STATE_H
