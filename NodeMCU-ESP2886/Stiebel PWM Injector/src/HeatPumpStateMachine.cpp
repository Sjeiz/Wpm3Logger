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
    startupInitialTemp(0.0),
    startupInitialTempSet(false),
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
  
  if (currentState == HOT_WATER || currentState == HEATING) {
    updateDefrostDetection(dutyCycle, temperature, tempAvailable);
  } else if (inDefrostCycle) {
    inDefrostCycle = false;
    if (eventCallback) {
      eventCallback("DEFROST CYCLE FINISHED");
    }
  }
  
  switch (currentState) {
    case STANDBY:
      if (pwmActive) {
        transitionToState(STARTUP, "HEATPUMP STARTED: Determining mode for 2 minutes...");
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
    startupInitialTempSet = false;
    return;
  }
  
  // Record initial temperature at startup
  if (!startupInitialTempSet && tempAvailable) {
    startupInitialTemp = temperature;
    startupInitialTempSet = true;
  }
  
  // Check if startup period has elapsed
  unsigned long elapsed = millis() - startupStartTime;
  if (elapsed >= STARTUP_WAIT_TIME) {
    // Determine mode based on temperature trend and threshold
    if (tempAvailable && startupInitialTempSet) {
      float tempChange = temperature - startupInitialTemp;
      
      if (temperature > TEMP_THRESHOLD_HOT_WATER) {
        transitionToState(HOT_WATER, "MODE DETECTED: Hot Water Mode");
      } else if (tempChange <= TEMP_TREND_COOLING_THRESHOLD) {
        transitionToState(COOLING, "MODE DETECTED: Cooling Mode (temp trend)");
      } else {
        transitionToState(HEATING, "MODE DETECTED: Heating Mode");
      }
    } else {
      transitionToState(HEATING, "MODE DETECTED: Heating Mode (temp sensor unavailable)");
    }
    
    startupInitialTempSet = false;
  }
}

// Handle operational states (HOT_WATER, HEATING, COOLING)
void HeatPumpStateMachine::handleOperationalState(bool pwmActive, float temperature) {
  if (!pwmActive) {
    if (currentState == HOT_WATER) {
      transitionToState(POST_RUN, "HEATPUMP STOPPED: Post-Run Timer started (30.0 min)");
      postRunStartTime = millis();
    } else {
      transitionToState(STANDBY, "HEATPUMP STOPPED: Entering Standby");
    }
    return;
  }
  
  if (!tempSensorAvailable) return;
  
  // Only HOT_WATER mode can switch at runtime (HEATING/COOLING remain fixed)
  if (currentState == HOT_WATER) {
    if (temperature < TEMP_THRESHOLD_HOT_WATER) {
      transitionToState(HEATING, "MODE CHANGE: Hot Water → Heating");
    }
  } else if (temperature > TEMP_THRESHOLD_HOT_WATER) {
    char eventMsg[80];
    snprintf(eventMsg, sizeof(eventMsg), "MODE CHANGE: %s → Hot Water", 
             currentState == HEATING ? "Heating" : "Cooling");
    transitionToState(HOT_WATER, eventMsg);
  }
}

// Handle POST_RUN state logic
void HeatPumpStateMachine::handlePostRunState(bool pwmActive) {
  if (pwmActive) {
    transitionToState(STARTUP, "HEATPUMP RESTARTED: Post-Run Timer cancelled, determining mode for 2 minutes...");
    startupStartTime = millis();
    return;
  }
  
  if (millis() - postRunStartTime >= POSTRUN_TIMER_DURATION) {
    transitionToState(STANDBY, "POST-RUN TIMER FINISHED: Entering Standby");
  }
}

// Update defrost cycle detection
void HeatPumpStateMachine::updateDefrostDetection(float dutyCycle, float temperature, bool tempAvailable) {
  if (currentState != HOT_WATER && currentState != HEATING) return;
  
  bool wasInDefrost = inDefrostCycle;
  
  if (dutyCycle >= DEFROST_DUTY_THRESHOLD && tempAvailable) {
    unsigned long currentTime = millis();
    
    if (lastTempUpdateTime > 0 && (currentTime - lastTempUpdateTime) >= 30000) {
      float tempChange = temperature - previousTemperature;
      inDefrostCycle = (tempChange < -0.3);
      
      previousTemperature = temperature;
      lastTempUpdateTime = currentTime;
    } else if (lastTempUpdateTime == 0) {
      previousTemperature = temperature;
      lastTempUpdateTime = currentTime;
    }
  } else {
    inDefrostCycle = false;
  }
  
  if (inDefrostCycle != wasInDefrost && eventCallback) {
    eventCallback(inDefrostCycle ? "DEFROST CYCLE STARTED" : "DEFROST CYCLE FINISHED");
  }
}

// Transition to new state with event logging
void HeatPumpStateMachine::transitionToState(HeatPumpState newState, const char* eventMessage) {
  previousState = currentState;
  currentState = newState;
  
  if (newState == STARTUP) {
    startupInitialTempSet = false;
  }
  
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
