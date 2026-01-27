#include "StateManager.h"

StateManager::StateManager(State* initialState) : currentState(initialState) {
    if (currentState) currentState->enter();
}

StateManager::~StateManager() {
    if (currentState) currentState->exit();
    delete currentState;
}

void StateManager::update(uint16_t modbusStatus) {
    State* nextState = currentState->transition(modbusStatus);
    if (nextState != currentState) {
        currentState->exit();
        delete currentState;
        currentState = nextState;
        currentState->enter();
    }
    currentState->handle();
}

const char* StateManager::currentStateName() const {
    return currentState ? currentState->name() : "UNKNOWN";
}
