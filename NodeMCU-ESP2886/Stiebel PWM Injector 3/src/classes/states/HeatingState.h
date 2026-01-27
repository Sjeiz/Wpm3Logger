#ifndef HEATINGSTATE_H
#define HEATINGSTATE_H

#include "State.h"

class HeatingState : public State {
public:
    void enter() override { Serial.println("Entering HEATING"); }
    void handle() override { /* Heating logic */ }
    void exit() override { Serial.println("Exiting HEATING"); }
    const char* name() const override { return "HEATING"; }
    State* transition(uint16_t modbusStatus) override;
};

#endif // HEATINGSTATE_H
