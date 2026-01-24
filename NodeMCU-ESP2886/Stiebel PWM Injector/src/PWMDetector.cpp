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
      dutyIn = (highTime * 100.0) / (highTime + lowTime);
      lastValidMeasurement = millis();
    }
  } else {
    // No update in configured timeout = no PWM signal
    unsigned long timeSinceLastValid = millis() - lastValidMeasurement;
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
