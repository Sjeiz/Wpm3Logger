
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
// Central transition function.
// isgStatus is web ID 471 OPERATING STATUS (BETRIEBSSTATUS), the bit field HA shows as sensor.isg_471;
// the ISG_471_* bit names and their meaning are in config.h.
State* centralTransition(uint16_t isgStatus, State* previousState) {
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

    // ERROR overrules everything: no value for web ID 471 (request failed or value absent)
    if (isgStatus == ISG_READ_ERROR) {
        postRunEndTime = 0;
        return errorState;
    }

    // DEFROST overrules everything except ERROR: 471 bit 9 DEFROST (ABTAUBETRIEB)
    if (isgStatus & ISG_471_DEFROST) {
        postRunEndTime = 0;
        return defrostState;
    }

    // Compressor active? 471 bit 6 COMPRESSOR (VERDICHTER)
    bool compressorAan = (isgStatus & ISG_471_COMPRESSOR);

    // Minimaal minStateTime in elke status blijven, behalve bij errorState
    if (previousState != errorState && (now - stateEnterTime) < minStateTime) {
        return previousState;
    }

    // HOT_WATER → POST_RUN when compressor turns off, only outside summer mode
    // (471 bit 7 SUMMER MODE, SOMMERBETRIEB).
    // In summer mode this falls through to STANDBY below.
    bool summerMode = (isgStatus & ISG_471_SUMMER_MODE);
    if (!compressorAan && previousState == hotWaterState && !summerMode) {
        postRunEndTime = millis() + postRunTimeout;
        return postRunState;
    }

    // POST_RUN → STANDBY when timer expires; a new run (471 bit 8 COOLING MODE, bit 4 HEATING or
    // bit 5 DHW MODE) cancels the timer
    if (previousState == postRunState) {
        if (isgStatus & (ISG_471_COOLING_MODE | ISG_471_HEATING | ISG_471_DHW_MODE)) {
            postRunEndTime = 0; // Cancel timer
            if (isgStatus & ISG_471_COOLING_MODE) return coolingState;  // bit 8 KUEHLBETRIEB
            if (isgStatus & ISG_471_HEATING)      return heatingState;  // bit 4 HEIZBETRIEB
            if (isgStatus & ISG_471_DHW_MODE)     return hotWaterState; // bit 5 WARMWASSERBETRIEB
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
    if (isgStatus & ISG_471_COOLING_MODE) return coolingState;  // bit 8 COOLING MODE (KUEHLBETRIEB)
    if (isgStatus & ISG_471_HEATING)      return heatingState;  // bit 4 HEATING (HEIZBETRIEB)
    if (isgStatus & ISG_471_DHW_MODE)     return hotWaterState; // bit 5 DHW MODE (WARMWASSERBETRIEB)

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

void StateManager::update(uint16_t isgStatus) {
    State* nextState = currentState->transition(isgStatus);
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
            stateChangeCallback(nextState->name(), isgStatus);
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
