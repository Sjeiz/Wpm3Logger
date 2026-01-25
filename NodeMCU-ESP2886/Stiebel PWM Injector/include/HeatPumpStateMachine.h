#ifndef HEATPUMP_STATE_MACHINE_H
#define HEATPUMP_STATE_MACHINE_H

#include <Arduino.h>

// Heat pump operational states
enum HeatPumpState {
  STANDBY,      // PWM-in OFF, no post-run timer
  STARTUP,      // PWM-in ON, determining mode (2 min)
  HOT_WATER,    // PWM-in ON, temp >40°C (domestic hot water)
  HEATING,      // PWM-in ON, stable temp (space heating)
  COOLING,      // PWM-in ON, falling temp (active cooling)
  POST_RUN      // PWM-in OFF, post-run timer active (30 min)
};

// Pump control modes
enum PumpControlMode {
  PUMP_NORMAL,   // Normal operation (both relays LOW)
  PUMP_BLOCKED,  // Blocked during defrost (D8=HIGH, D7=LOW)
  PUMP_FORCED    // Forced ON during post-run (D7=HIGH, D8=LOW)
};

class HeatPumpStateMachine {
private:
  HeatPumpState currentState;
  HeatPumpState previousState;
  bool inDefrostCycle;
  unsigned long startupStartTime;
  unsigned long postRunStartTime;
  bool tempSensorAvailable;
  float lastTemperature;
  float previousTemperature;
  unsigned long lastTempUpdateTime;
  float startupInitialTemp;
  bool startupInitialTempSet;
  
  // Callback for state change events
  void (*eventCallback)(const char* event);
  
  // Internal helper methods
  void transitionToState(HeatPumpState newState, const char* eventMessage);
  void updateDefrostDetection(float dutyCycle, float temperature, bool tempAvailable);
  void handleStartupState(bool pwmActive, float temperature, bool tempAvailable);
  void handleOperationalState(bool pwmActive, float temperature);
  void handlePostRunState(bool pwmActive);

public:
  HeatPumpStateMachine();
  
  // Initialization
  void begin();
  
  // Main update method - call every loop iteration
  void update(bool pwmActive, float dutyCycle, float temperature, bool tempSensorAvailable);
  
  // State queries
  HeatPumpState getCurrentState() const;
  String getStateString() const;
  bool isInDefrostCycle() const;
  
  // Pump control
  PumpControlMode getPumpControlMode() const;
  String getPumpStatusString() const;
  
  // Timer information
  float getStartupTimeRemaining() const;  // Returns remaining startup time in minutes
  float getPostRunTimeRemaining() const;  // Returns remaining post-run time in minutes
  
  // Event callback registration
  void setEventCallback(void (*callback)(const char* event));
};

#endif
