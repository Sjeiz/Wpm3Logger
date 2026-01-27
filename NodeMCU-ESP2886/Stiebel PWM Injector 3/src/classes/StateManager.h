#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include "State.h"

class StateManager {
public:
    StateManager(State* initialState);
    ~StateManager();
    void update(uint16_t modbusStatus);
    const char* currentStateName() const;
private:
    State* currentState;
};

#endif // STATEMANAGER_H
