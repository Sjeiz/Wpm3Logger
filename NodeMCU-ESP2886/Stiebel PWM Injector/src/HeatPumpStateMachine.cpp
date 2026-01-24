#include "HeatPumpStateMachine.h"
#include "Config.h"

HeatPumpStateMachine::HeatPumpStateMachine() 
  : currentState(STANDBY),
    previousState(STANDBY),
    inDefrostCycle(false),
    startupStartTime(0),
    postRunStartTime(0),
    tempSensorAvailable(false),
    lastTemperature(0.0),
    previousTemperature(0.0),
    lastTempUpdateTime(0),
    eventCallback(nullptr) {
}

void HeatPumpStateMachine::begin() {
  currentState = STANDBY;
  previousState = STANDBY;
  inDefrostCycle = false;
  startupStartTime = 0;
  postRunStartTime = 0;
}

// Main update method - call every loop iteration
void HeatPumpStateMachine::update(bool pwmActive, float dutyCycle, float temperature, bool tempAvailable) {
  tempSensorAvailable = tempAvailable;
  lastTemperature = temperature;
  
  // Update defrost detection (only in HOT_WATER or HEATING states)
  if (currentState == HOT_WATER || currentState == HEATING) {
    updateDefrostDetection(dutyCycle, temperature, tempAvailable);
  } else {
    // Not in a state where defrost is applicable
    if (inDefrostCycle) {
      inDefrostCycle = false;
      if (eventCallback) {
        eventCallback("DEFROST CYCLE FINISHED");
      }
    }
  }
  
  // State-specific handling
  switch (currentState) {
    case STANDBY:
      if (pwmActive) {
        transitionToState(STARTUP, "HEATPUMP STARTED: Determining mode for 5 minutes...");
        startupStartTime = millis();
      }
      break;
      
    case STARTUP:
      handleStartupState(pwmActive, temperature, tempAvailable);
      break;
      
    case HOT_WATER:
    case HEATING:
    case COOLING:
      handleOperationalState(pwmActive, temperature);
      break;
      
    case POST_RUN:
      handlePostRunState(pwmActive);
      break;
  }
}

// Handle STARTUP state logic
void HeatPumpStateMachine::handleStartupState(bool pwmActive, float temperature, bool tempAvailable) {
  if (!pwmActive) {
    // PWM turned off during startup
    transitionToState(STANDBY, "HEATPUMP STOPPED: Entering Standby");
    return;
  }
  
  // Check if startup period has elapsed
  unsigned long elapsed = millis() - startupStartTime;
  if (elapsed >= STARTUP_WAIT_TIME) {
    // Determine mode based on temperature
    HeatPumpState newState = determineStateFromTemperature(temperature);
    
    // Generate appropriate event message
    if (newState == HOT_WATER) {
      if (tempAvailable) {
        transitionToState(HOT_WATER, "MODE DETECTED: Hot Water Mode");
      } else {
        transitionToState(HOT_WATER, "MODE DETECTED: Hot Water Mode (temp sensor unavailable)");
      }
    } else if (newState == HEATING) {
      transitionToState(HEATING, "MODE DETECTED: Heating Mode");
    } else {  // COOLING
      transitionToState(COOLING, "MODE DETECTED: Cooling Mode");
    }
  }
}

// Handle operational states (HOT_WATER, HEATING, COOLING)
void HeatPumpStateMachine::handleOperationalState(bool pwmActive, float temperature) {
  if (!pwmActive) {
    // PWM turned off
    if (currentState == HOT_WATER) {
      // Only trigger post-run from HOT_WATER state
      transitionToState(POST_RUN, "HEATPUMP STOPPED: Post-Run Timer started (30.0 min)");
      postRunStartTime = millis();
    } else {
      // From HEATING or COOLING, go directly to STANDBY
      transitionToState(STANDBY, "HEATPUMP STOPPED: Entering Standby");
    }
    return;
  }
  
  // PWM still active - check for mode changes based on temperature
  HeatPumpState newState = determineStateFromTemperature(temperature);
  
  if (newState != currentState) {
    // Mode change detected
    char eventMsg[100];
    const char* fromState = (currentState == HOT_WATER) ? "Hot Water" : 
                            (currentState == HEATING) ? "Heating" : "Cooling";
    const char* toState = (newState == HOT_WATER) ? "Hot Water" : 
                          (newState == HEATING) ? "Heating" : "Cooling";
    
    snprintf(eventMsg, sizeof(eventMsg), "MODE CHANGE: %s → %s", fromState, toState);
    transitionToState(newState, eventMsg);
  }
}

// Handle POST_RUN state logic
void HeatPumpStateMachine::handlePostRunState(bool pwmActive) {
  if (pwmActive) {
    // PWM returned during post-run - cancel timer and restart
    transitionToState(STARTUP, "HEATPUMP RESTARTED: Post-Run Timer cancelled, determining mode for 5 minutes...");
    startupStartTime = millis();
    return;
  }
  
  // Check if post-run timer has expired
  unsigned long elapsed = millis() - postRunStartTime;
  if (elapsed >= POSTRUN_TIMER_DURATION) {
    transitionToState(STANDBY, "POST-RUN TIMER FINISHED: Entering Standby");
  }
}

// Determine state based on temperature thresholds
HeatPumpState HeatPumpStateMachine::determineStateFromTemperature(float temperature) {
  // If temperature sensor unavailable, default to HOT_WATER (conservative choice)
  if (!tempSensorAvailable) {
    return HOT_WATER;
  }
  
  // Temperature evaluation order (as specified):
  // 1. Check >45°C first (HOT_WATER)
  // 2. Then check ≥24°C (HEATING)
  // 3. Otherwise <24°C (COOLING)
  if (temperature > TEMP_THRESHOLD_HOT_WATER) {
    return HOT_WATER;
  } else if (temperature >= TEMP_THRESHOLD_HEATING) {
    return HEATING;
  } else {
    return COOLING;
  }
}

// Update defrost cycle detection
void HeatPumpStateMachine::updateDefrostDetection(float dutyCycle, float temperature, bool tempAvailable) {
  // Defrost only applicable in HOT_WATER or HEATING states
  if (currentState != HOT_WATER && currentState != HEATING) {
    return;
  }
  
  bool wasInDefrost = inDefrostCycle;
  
  // Defrost detection requires:
  // 1. High duty cycle (≥95%)
  // 2. Temperature sensor available
  // 3. Falling temperature (indicating defrost cycle)
  if (dutyCycle >= DEFROST_DUTY_THRESHOLD && tempAvailable) {
    unsigned long currentTime = millis();
    
    // Check if we have a previous temperature measurement (wait at least 30 seconds for trend)
    if (lastTempUpdateTime > 0 && (currentTime - lastTempUpdateTime) >= 30000) {
      float tempChange = temperature - previousTemperature;
      
      // Defrost: temperature is falling (< -0.3°C change over 30 second period)
      // Normal hot water: temperature is rising or stable
      if (tempChange < -0.3) {
        inDefrostCycle = true;
      } else {
        inDefrostCycle = false;
      }
      
      // Update temperature tracking
      previousTemperature = temperature;
      lastTempUpdateTime = currentTime;
    } else {
      // First measurement or too soon - initialize tracking
      if (lastTempUpdateTime == 0) {
        previousTemperature = temperature;
        lastTempUpdateTime = currentTime;
      }
      // Keep current defrost state until we have trend data
    }
  } else {
    // Duty cycle below threshold or temp sensor unavailable - not in defrost
    inDefrostCycle = false;
  }
  
  // Log state change
  if (inDefrostCycle && !wasInDefrost) {
    if (eventCallback) {
      eventCallback("DEFROST CYCLE STARTED");
    }
  } else if (!inDefrostCycle && wasInDefrost) {
    if (eventCallback) {
      eventCallback("DEFROST CYCLE FINISHED");
    }
  }
}

// Transition to new state with event logging
void HeatPumpStateMachine::transitionToState(HeatPumpState newState, const char* eventMessage) {
  previousState = currentState;
  currentState = newState;
  
  if (eventCallback && eventMessage) {
    eventCallback(eventMessage);
  }
}

// Get current state
HeatPumpState HeatPumpStateMachine::getCurrentState() const {
  return currentState;
}

// Get state as string for logging
String HeatPumpStateMachine::getStateString() const {
  switch (currentState) {
    case STANDBY:
      return "STANDBY";
    case STARTUP:
      return "STARTUP";
    case HOT_WATER:
      return inDefrostCycle ? "HOT_WATER (defrosting)" : "HOT_WATER";
    case HEATING:
      return inDefrostCycle ? "HEATING (defrosting)" : "HEATING";
    case COOLING:
      return "COOLING";
    case POST_RUN: {
      float remaining = getPostRunTimeRemaining();
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "POST_RUN (%.1f min)", remaining);
      return String(buffer);
    }
    default:
      return "UNKNOWN";
  }
}

// Check if in defrost cycle
bool HeatPumpStateMachine::isInDefrostCycle() const {
  return inDefrostCycle;
}

// Get pump control mode
PumpControlMode HeatPumpStateMachine::getPumpControlMode() const {
  // Priority: POST_RUN > defrost > normal
  if (currentState == POST_RUN) {
    return PUMP_FORCED;
  } else if (inDefrostCycle) {
    return PUMP_BLOCKED;
  } else {
    return PUMP_NORMAL;
  }
}

// Get pump status as string
String HeatPumpStateMachine::getPumpStatusString() const {
  switch (getPumpControlMode()) {
    case PUMP_NORMAL:
      return "NORMAL";
    case PUMP_BLOCKED:
      return "BLOCKED";
    case PUMP_FORCED:
      return "FORCED";
    default:
      return "UNKNOWN";
  }
}

// Get remaining startup time in minutes
float HeatPumpStateMachine::getStartupTimeRemaining() const {
  if (currentState != STARTUP) {
    return 0.0;
  }
  
  unsigned long elapsed = millis() - startupStartTime;
  if (elapsed >= STARTUP_WAIT_TIME) {
    return 0.0;
  }
  
  unsigned long remaining = STARTUP_WAIT_TIME - elapsed;
  return remaining / 60000.0;  // Convert to minutes
}

// Get remaining post-run time in minutes
float HeatPumpStateMachine::getPostRunTimeRemaining() const {
  if (currentState != POST_RUN) {
    return 0.0;
  }
  
  unsigned long elapsed = millis() - postRunStartTime;
  if (elapsed >= POSTRUN_TIMER_DURATION) {
    return 0.0;
  }
  
  unsigned long remaining = POSTRUN_TIMER_DURATION - elapsed;
  return remaining / 60000.0;  // Convert to minutes
}

// Register event callback
void HeatPumpStateMachine::setEventCallback(void (*callback)(const char* event)) {
  eventCallback = callback;
}
