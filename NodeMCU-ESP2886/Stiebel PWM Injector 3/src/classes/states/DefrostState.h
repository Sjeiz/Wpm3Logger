#ifndef DEFROSTSTATE_H
#define DEFROSTSTATE_H

#include "State.h"

class DefrostState : public State {
public:
    void enter() override { Serial.println("Entering DEFROST"); }
    void handle() override { /* Defrost logic */ }
    void exit() override { Serial.println("Exiting DEFROST"); }
    const char* name() const override { return "DEFROST"; }
    State* transition(uint16_t modbusStatus) override;
};

#endif // DEFROSTSTATE_H
