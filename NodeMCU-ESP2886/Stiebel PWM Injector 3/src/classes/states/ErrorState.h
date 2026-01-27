#ifndef ERRORSTATE_H
#define ERRORSTATE_H

#include "State.h"

class ErrorState : public State {
public:
    void enter() override { Serial.println("Entering ERROR"); }
    void handle() override { /* Error logic */ }
    void exit() override { Serial.println("Exiting ERROR"); }
    const char* name() const override { return "ERROR"; }
    State* transition(uint16_t modbusStatus) override { return this; }
};

#endif // ERRORSTATE_H
