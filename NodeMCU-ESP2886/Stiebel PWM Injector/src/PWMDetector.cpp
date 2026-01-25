#include "PWMDetector.h"
#include "Config.h"

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
  
  // Track valid edges only (after debounce and level change checks)
  lastAnyEdgeTime = millis();

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
  static unsigned long lastDebugUpdate = 0;
  unsigned long currentTime = millis();
  
  // Periodic status debug every 15 seconds, only when having issues
  if (DEBUG_MODE && (currentTime - lastDebugUpdate >= 15000)) {
    unsigned long timeSinceEdge = currentTime - lastAnyEdgeTime;
    unsigned long timeSinceValid = currentTime - lastValidMeasurement;
    // Only log if we're approaching timeout or in problematic state
    if (timeSinceValid > 5000 || timeSinceEdge > 500) {
      Serial.print("DEBUG PWM: pwmValid=");
      Serial.print(pwmValid ? "YES" : "NO");
      Serial.print(" lastEdge=");
      Serial.print(timeSinceEdge);
      Serial.print("ms lastValid=");
      Serial.print(timeSinceValid);
      Serial.print("ms pinLevel=");
      Serial.print(digitalRead(pwmPin) == LOW ? "LOW" : "HIGH");
      Serial.print(" detected=");
      Serial.println(pwmDetected ? "YES" : "NO");
    }
    lastDebugUpdate = currentTime;
  }
  
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
    bool currentLevel = digitalRead(pwmPin);
    
    // Only consider DC if no edges for NO_EDGE_THRESHOLD (prevents ruis triggering)
    unsigned long timeSinceLastAnyEdge = currentTime - lastAnyEdgeTime;
    if (timeSinceLastAnyEdge < NO_EDGE_THRESHOLD) {
      // Still seeing edge activity, don't try DC detection yet
      // BUT: keep updating lastValidMeasurement if we're currently detecting PWM
      // This prevents timeout during the transition to DC level
      if (pwmDetected) {
        lastValidMeasurement = currentTime;
      }
      return;
    }
    
    // Check if level is stable
    if (currentLevel == lastDCLevel) {
      unsigned long dcDuration = currentTime - dcLevelStartTime;
      
      // CRITICAL: Update lastValidMeasurement immediately for LOW level
      // This prevents timeout during the DC detection wait period
      if (currentLevel == LOW && dcDuration > 100) {
        lastValidMeasurement = currentTime;
      }
      
      if (dcDuration > DC_DETECTION_THRESHOLD) {
        // Open collector inverts logic: LOW = active, HIGH = inactive
        // Constant LOW level = 100% duty (pump fully active)
        // Constant HIGH level = 0% duty (no signal)
        if (currentLevel == LOW) {
          // Constant LOW = 100% duty (defrost mode)
          if (!pwmDetected || dutyIn != 100.0) {
            pwmDetected = true;
            freqIn = 0.0;  // DC has no frequency
            dutyIn = 100.0;
            lastValidMeasurement = currentTime;  // Keep updating to prevent timeout
            if (DEBUG_MODE) {
              Serial.println("DEBUG: PWM DC level - constant LOW = 100% duty (defrost)");
            }
          } else {
            // Already detected at 100%, keep updating lastValidMeasurement to prevent timeout
            lastValidMeasurement = currentTime;
          }
        } else {
          // HIGH = 0% = no signal (only if we haven't timed out yet)
          unsigned long timeSinceLastValid = currentTime - lastValidMeasurement;
          if (timeSinceLastValid > PWM_DETECTION_TIMEOUT) {
            // Timed out - truly no signal
            if (pwmDetected) {
              pwmDetected = false;
              freqIn = 0.0;
              dutyIn = 0.0;
              if (DEBUG_MODE) {
                Serial.print("DEBUG: PWM timeout - constant HIGH for ");
                Serial.print(timeSinceLastValid);
                Serial.println("ms, no signal detected");
              }
            }
          }
        }
      }
    } else {
      // Level changed, reset DC timer
      if (DEBUG_MODE && (currentTime - dcLevelStartTime > 100)) {
        Serial.print("DEBUG: PWM level changed from ");
        Serial.print(lastDCLevel == LOW ? "LOW" : "HIGH");
        Serial.print(" to ");
        Serial.print(currentLevel == LOW ? "LOW" : "HIGH");
        Serial.print(" after ");
        Serial.print(currentTime - dcLevelStartTime);
        Serial.println("ms");
      }
      lastDCLevel = currentLevel;
      dcLevelStartTime = currentTime;
    }
  }
  
  // Check for state change
  pwmStateJustChanged = false;
  if (pwmDetected != lastPwmDetectedState) {
    pwmStateJustChanged = true;
    lastPwmDetectedState = pwmDetected;
    if (DEBUG_MODE) {
      Serial.print("DEBUG: PWM state changed to ");
      Serial.print(pwmDetected ? "DETECTED" : "NOT DETECTED");
      Serial.print(" - Duty: ");
      Serial.print(dutyIn, 1);
      Serial.print("%, Freq: ");
      Serial.print(freqIn, 1);
      Serial.println("Hz");
    }
  }
}

// Get PWM detection state change
bool pwmStateChanged() {
  return pwmStateJustChanged;
}
