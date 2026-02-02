
#include "StateManager.h"
#include "loggers/Logger.h"
#include "config.h"

// External log function from main.cpp
extern void logMessage(const String& message, const LogLevel level);


// Global state instances (only defined here, declared extern elsewhere)
State* errorState      = nullptr;
State* defrostState    = nullptr;
State* postRunState    = nullptr;
State* coolingState    = nullptr;
State* heatingState    = nullptr;
State* hotWaterState   = nullptr;
State* standbyState    = nullptr;

// postRunEndTime moet extern gedeeld worden voor correcte logging
unsigned long postRunEndTime = 0;
// Central transition function
State* centralTransition(uint16_t modbusStatus, State* previousState) {
    const unsigned long postRunTimeout = POST_RUN_DURATION_MIN * 60000UL;
    const unsigned long minStateTime = ISG_POLL_STABLETIME_SEC * 1000UL;
    static unsigned long stateEnterTime = 0;
    static State* lastState = nullptr;
    unsigned long now = millis();

    // Detect state change
    if (previousState != lastState) {
        stateEnterTime = now;
        lastState = previousState;
    }

    // ERROR overrules everything
    if (modbusStatus == ISG_MODBUS_READ_ERROR) {
        postRunEndTime = 0;
        return errorState;
    }

    // DEFROST overrules everything except ERROR
    if (modbusStatus & ISG_STATUS_DEFROSTING) {
        postRunEndTime = 0;
        return defrostState;
    }

    // Compressor active?
    bool compressorAan = (modbusStatus & ISG_STATUS_COMPRESSOR);

    // Minimaal minStateTime in elke status blijven
    if ((now - stateEnterTime) < minStateTime) {
        return previousState;
    }

    // HOT_WATER → POST_RUN when compressor turns off
    if (!compressorAan && previousState == hotWaterState) {
        postRunEndTime = millis() + postRunTimeout;
        return postRunState;
    }

    // POST_RUN → STANDBY when timer expires or compressor turns on again
    if (previousState == postRunState) {
        if (modbusStatus & (ISG_STATUS_COOLING | ISG_STATUS_HEATING | ISG_STATUS_HOT_WATER)) {
            postRunEndTime = 0; // Cancel timer
            if (modbusStatus & ISG_STATUS_COOLING)    return coolingState;
            if (modbusStatus & ISG_STATUS_HEATING)    return heatingState;
            if (modbusStatus & ISG_STATUS_HOT_WATER)  return hotWaterState;
        }
        if (postRunEndTime != 0 && millis() < postRunEndTime) {
            return postRunState; // Stay in POST_RUN
        }
        postRunEndTime = 0;
        return standbyState; // Timer expired
    }

    // Compressor off: always go to STANDBY
    if (!compressorAan) return standbyState;

    // Compressor on: normal priority
    if (modbusStatus & ISG_STATUS_COOLING)    return coolingState;
    if (modbusStatus & ISG_STATUS_HEATING)    return heatingState;
    if (modbusStatus & ISG_STATUS_HOT_WATER)  return hotWaterState;

    // Fallback
    return standbyState;
}


StateManager::StateManager() {
    // Initialize global state pointers only once
    if (!errorState)      errorState    = new BaseState("ERROR", [this](uint16_t status) { return centralTransition(status, errorState); });
    if (!defrostState)    defrostState  = new BaseState("DEFROST", [this](uint16_t status) { return centralTransition(status, defrostState); });
    if (!postRunState)    postRunState  = new BaseState("POST_RUN", [this](uint16_t status) { return centralTransition(status, postRunState); });
    if (!coolingState)    coolingState  = new BaseState("COOLING", [this](uint16_t status) { return centralTransition(status, coolingState); });
    if (!heatingState)    heatingState  = new BaseState("HEATING", [this](uint16_t status) { return centralTransition(status, heatingState); });
    if (!hotWaterState)   hotWaterState = new BaseState("HOT_WATER", [this](uint16_t status) { return centralTransition(status, hotWaterState); });
    if (!standbyState)    standbyState  = new BaseState("STANDBY", [this](uint16_t status) { return centralTransition(status, standbyState); });
}

void StateManager::begin(State* initialState) {
    if (currentState) currentState->exit();
    currentState = initialState;
    if (currentState) currentState->enter();
}

StateManager::~StateManager() {
    if (currentState) currentState->exit();
    delete currentState;
}

// stateEnterTime is declared externally in main.cpp
extern unsigned long stateEnterTime;

void StateManager::update(uint16_t modbusStatus) {
    State* nextState = currentState->transition(modbusStatus);
    // Debug logging: show pointer and name values before and after transition
    Serial.printf("[DEBUG] update() called. currentState ptr: %p, nextState ptr: %p\n", currentState, nextState);
    if (currentState) {
        Serial.printf("[DEBUG] currentState name: %s\n", currentState->name());
    }
    if (nextState) {
        Serial.printf("[DEBUG] nextState name: %s\n", nextState->name());
    }
    if (nextState != currentState) {
        unsigned long now = millis();
        unsigned long lastStateDuration = now - stateEnterTime;
        // Smart time display
        char durationStr[24] = "";
        if (lastStateDuration < 60000UL) {
            snprintf(durationStr, sizeof(durationStr), "%lu sec", lastStateDuration / 1000UL);
        } else if (lastStateDuration < 3600000UL) {
            snprintf(durationStr, sizeof(durationStr), "%.1f min", lastStateDuration / 60000.0f);
        } else {
            unsigned long totalSec = lastStateDuration / 1000UL;
            unsigned long hours = totalSec / 3600;
            unsigned long mins = (totalSec % 3600) / 60;
            unsigned long secs = totalSec % 60;
            snprintf(durationStr, sizeof(durationStr), "%lu:%02lu:%02lu", hours, mins, secs);
        }

        char logLine[200];
        if (strcmp(nextState->name(), "POST_RUN") == 0) {
            float min = 0.0f;
            if (postRunEndTime > millis()) {
                min = (postRunEndTime - millis()) / 60000.0f;
            }
            snprintf(logLine, sizeof(logLine),
                "StateChange: %s → %s (%s in last state. Starting timer for %.1f minutes) ====================",
                currentState->name(), nextState->name(), durationStr, min);
        } else {
            snprintf(logLine, sizeof(logLine),
                "StateChange: %s → %s (%s in last state) ====================",
                currentState->name(), nextState->name(), durationStr);
        }
        logMessage(logLine, LogLevel::LOG_NORMAL);
        if (DEBUG) {
            char exitBuf[64];
            snprintf(exitBuf, sizeof(exitBuf), "Exiting %s", currentState->name());
            logMessage(exitBuf, LogLevel::LOG_DEBUG);
        }
        // Callback voor state change
        if (stateChangeCallback) {
            stateChangeCallback(nextState->name(), modbusStatus);
        }
        currentState->exit();
        if (DEBUG) {
            char enterBuf[64];
            snprintf(enterBuf, sizeof(enterBuf), "Entering %s", nextState->name());
            logMessage(enterBuf, LogLevel::LOG_DEBUG);
        }
        currentState = nextState;
        currentState->enter();
        stateEnterTime = now;
        Serial.printf("[DEBUG] State transition detected. stateEnterTime set to: %lu\n", stateEnterTime);
    } else {
        Serial.printf("[DEBUG] No state transition. stateEnterTime unchanged: %lu\n", stateEnterTime);
    }
    currentState->handle();
}

const char* StateManager::currentStateName() const {
    return currentState ? currentState->name() : "UNKNOWN";
}
