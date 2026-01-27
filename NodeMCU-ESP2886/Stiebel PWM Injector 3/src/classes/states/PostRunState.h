#ifndef POSTRUNSTATE_H
#define POSTRUNSTATE_H

#include "State.h"

class PostRunState : public State {
public:
    void enter() override { Serial.println("Entering POST_RUN"); }
    void handle() override { /* Post-run logic */ }
    void exit() override { Serial.println("Exiting POST_RUN"); }
    const char* name() const override { return "POST_RUN"; }
    State* transition(uint16_t modbusStatus) override;
};

#endif // POSTRUNSTATE_H
