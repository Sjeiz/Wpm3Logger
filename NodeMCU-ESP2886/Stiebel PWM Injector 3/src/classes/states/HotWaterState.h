#ifndef HOTWATERSTATE_H
#define HOTWATERSTATE_H

#include "State.h"

class HotWaterState : public State {
public:
    void enter() override { Serial.println("Entering HOT_WATER"); }
    void handle() override { /* Hot water logic */ }
    void exit() override { Serial.println("Exiting HOT_WATER"); }
    const char* name() const override { return "HOT_WATER"; }
    State* transition(uint16_t modbusStatus) override;
};

#endif // HOTWATERSTATE_H
