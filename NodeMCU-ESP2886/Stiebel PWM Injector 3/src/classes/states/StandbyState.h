#ifndef STANDBYSTATE_H
#define STANDBYSTATE_H

#include "State.h"

class StandbyState : public State {
public:
    void enter() override { Serial.println("Entering STANDBY"); }
    void handle() override { /* Standby logic */ }
    void exit() override { Serial.println("Exiting STANDBY"); }
    const char* name() const override { return "STANDBY"; }
    State* transition(uint16_t modbusStatus) override;
};

#endif // STANDBYSTATE_H
