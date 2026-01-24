#include "PWMDetector.h"

// PWM measurement with interrupt-based edge detection
volatile unsigned long lastRisingEdge = 0;
volatile unsigned long lastFallingEdge = 0;
volatile unsigned long highDuration = 0;
volatile unsigned long lowDuration = 0;
volatile bool pwmValid = false;
volatile unsigned long edgeCount = 0;

// PWM results
float dutyIn = 0.0;
float freqIn = 0.0;
bool pwmDetected = false;

// State
static int pwmPin = -1;
static unsigned long lastValidMeasurement = 0;
static bool lastPwmDetectedState = false;
static bool pwmStateJustChanged = false;

// DC level detection (for 0% or 100% duty)
static unsigned long dcLevelStartTime = 0;
static bool lastDCLevel = HIGH;
static unsigned long lastAnyEdgeTime = 0;  // Track any edge, including debounced
#define DC_DETECTION_THRESHOLD 500      // 500ms of constant level = DC signal
#define NO_EDGE_THRESHOLD 1000          // Must have no edges for 1 second before DC detection

// PWM Edge Detection Interrupt
IRAM_ATTR void handlePWMEdge() {
  static unsigned long lastEdgeTime = 0;
  static bool lastLevel = HIGH;
  unsigned long currentTime = micros();
  bool currentLevel = digitalRead(pwmPin);
  
  // Track ANY edge attempt (before debounce check)
  lastAnyEdgeTime = millis();
  
  // Debounce: ignore edges faster than configured time to filter optocoupler noise
  unsigned long timeSinceLastEdge = currentTime - lastEdgeTime;
  if (timeSinceLastEdge < PWM_DEBOUNCE_TIME) {
    return;  // Ignore bouncing
  }
  
  // Only process on actual level change
  if (currentLevel == lastLevel) {
    return;
  }
  lastLevel = currentLevel;
  edgeCount++;

  if (currentLevel == HIGH) {
    // Rising edge
    if (lastEdgeTime > 0) {
      lowDuration = currentTime - lastEdgeTime;
    }
    lastRisingEdge = currentTime;
  } else {
    // Falling edge
    highDuration = currentTime - lastRisingEdge;
    lastFallingEdge = currentTime;
    pwmValid = true;  // Mark measurement as valid
  }

  lastEdgeTime = currentTime;
}

// Initialize PWM detection
void initPWMDetector(int pin) {
  pwmPin = pin;
  pinMode(pwmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pwmPin), handlePWMEdge, CHANGE);
  lastValidMeasurement = millis();
  lastAnyEdgeTime = millis();
  dcLevelStartTime = millis();
  lastDCLevel = digitalRead(pin);
}

// Update PWM measurements from interrupt data
void updatePWMDetection() {
  // Check if we have a valid PWM measurement from interrupt
  if (pwmValid) {
    noInterrupts();
    unsigned long highTime = highDuration;
    unsigned long lowTime = lowDuration;
    pwmValid = false;
    interrupts();

    if (highTime > 0 && lowTime > 0) {
      pwmDetected = true;
      float period = (highTime + lowTime) / 1000000.0;
      freqIn = 1.0 / period;
      // Invert duty cycle for open collector optocoupler (LOW = active)
      dutyIn = 100.0 - (highTime * 100.0) / (highTime + lowTime);
      lastValidMeasurement = millis();
      dcLevelStartTime = millis();  // Reset DC counter on edge detection
    }
  } else {
    // No edge detected - check for DC level (constant 0% or 100%)
    unsigned long currentTime = millis();
    bool currentLevel = digitalRead(pwmPin);
    
    // Only consider DC if no edges for NO_EDGE_THRESHOLD (prevents ruis triggering)
    unsigned long timeSinceLastAnyEdge = currentTime - lastAnyEdgeTime;
    if (timeSinceLastAnyEdge < NO_EDGE_THRESHOLD) {
      // Still seeing edge activity, don't try DC detection yet
      return;
    }
    
    // Check if level is stable
    if (currentLevel == lastDCLevel) {
      unsigned long dcDuration = currentTime - dcLevelStartTime;
      if (dcDuration > DC_DETECTION_THRESHOLD && dcDuration < PWM_DETECTION_TIMEOUT) {
        // Open collector inverts logic: LOW = active, HIGH = inactive
        // Constant LOW level = 100% duty (pump fully active)
        // Constant HIGH level = 0% duty (no signal)
        if (currentLevel == LOW) {
          pwmDetected = true;
          freqIn = 0.0;  // DC has no frequency
          dutyIn = 100.0;
          lastValidMeasurement = currentTime;
        } else {
          // HIGH = 0% = no signal
          pwmDetected = false;
          freqIn = 0.0;
          dutyIn = 0.0;
        }
      }
    } else {
      // Level changed, reset DC timer
      lastDCLevel = currentLevel;
      dcLevelStartTime = currentTime;
    }
    
    // Check for timeout (no edges and not stable DC)
    unsigned long timeSinceLastValid = currentTime - lastValidMeasurement;
    if (timeSinceLastValid > PWM_DETECTION_TIMEOUT) {
      pwmDetected = false;
      freqIn = 0.0;
      dutyIn = 0.0;
    }
  }
  
  // Check for state change
  pwmStateJustChanged = false;
  if (pwmDetected != lastPwmDetectedState) {
    pwmStateJustChanged = true;
    lastPwmDetectedState = pwmDetected;
    
    if (pwmDetected) {
      Serial.println("PWM-in changed to ON");
    } else {
      Serial.println("PWM-in changed to OFF");
    }
  }
}

// Get PWM detection state change
bool pwmStateChanged() {
  return pwmStateJustChanged;
}
