#include "config.h"


#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <functional>
#include "../include/config.h"

// --- State interface & BaseState ---
class State {
public:
    virtual ~State() {}
    virtual void enter() = 0;
    virtual void handle() = 0;
    virtual void exit() = 0;
    virtual const char* name() const = 0;
    virtual State* transition(uint16_t modbusStatus) = 0;
};


// Externe centralTransition zodat test en src dezelfde implementatie delen
extern State* centralTransition(uint16_t modbusStatus, State* previousState);

// Globale state pointers zodat test en src dezelfde instanties delen
extern State* errorState;
extern State* defrostState;
extern State* postRunState;
extern State* coolingState;
extern State* heatingState;
extern State* hotWaterState;
extern State* standbyState;

class BaseState : public State {
public:
    using TransitionFunc = std::function<State*(uint16_t)>;

    BaseState(const char* stateName, TransitionFunc transitionFunc)
        : _transitionFunc(transitionFunc), _name(stateName) {}

    void enter() override {
        if (DEBUG) Serial.printf("[DEBUG]Entering %s\n", _name);
    }
    void handle() override { /* Optional: generic or injected logic */ }
    void exit() override {
        if (DEBUG) Serial.printf("[DEBUG] Exiting %s\n", _name);
    }
    const char* name() const override { return _name; }
    State* transition(uint16_t modbusStatus) override {
        return _transitionFunc ? _transitionFunc(modbusStatus) : this;
    }

    TransitionFunc _transitionFunc;
private:
    const char* _name;
};

// --- StateManager ---
class StateManager {
public:
    using StateChangeCallback = std::function<void(const char* newState, uint16_t modbusStatus)>;

    StateManager();
    ~StateManager();
    void begin(State* initialState);
    void update(uint16_t modbusStatus);
    const char* currentStateName() const;
    State* getCurrentStatePtr() const { return currentState; }
    void setStateChangeCallback(StateChangeCallback cb) { stateChangeCallback = cb; }
private:
    State* currentState = nullptr;
    StateChangeCallback stateChangeCallback = nullptr;
    uint16_t lastModbusStatus = 0;
};
