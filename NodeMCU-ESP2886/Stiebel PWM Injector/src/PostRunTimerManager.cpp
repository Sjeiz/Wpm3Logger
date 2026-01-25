#include "PostRunTimerManager.h"

// State
static unsigned long postRunTimerDuration = 2 * 60 * 1000;   // Default: 2 minutes (will be set via initPostRunTimer)

// Post-run timer state
static bool postRunTimerActive = false;
static unsigned long postRunTimerStartTime = 0;
static unsigned long postRunTimerEndTime = 0;
static int pumpPin = -1;

// Forward declaration - implemented in main.cpp
extern void setOutputPWM(int frequency, int duty);

// Initialize post-run timer manager
void initPostRunTimer(unsigned long durationMs) {
  postRunTimerDuration = durationMs;
  postRunTimerActive = false;
  postRunTimerStartTime = 0;
  postRunTimerEndTime = 0;
}

// Get the configured post-run timer duration
unsigned long getPostRunTimerDuration() {
  return postRunTimerDuration;
}

// Start post-run timer
void startPostRunTimer() {
  if (postRunTimerActive) {
    return;  // Already running
  }
  
  postRunTimerActive = true;
  postRunTimerStartTime = millis();
  postRunTimerEndTime = postRunTimerStartTime + postRunTimerDuration;
  
  // Set output PWM to post-run timer parameters
  setOutputPWM(PWM_OUTPUT_FREQ_POSTRUN, PWM_OUTPUT_DUTY_POSTRUN);
  
  // Activate pump
  if (pumpPin >= 0) {
    if (DEBUG_MODE) {
      Serial.print("DEBUG: Setting pin ");
      Serial.print(pumpPin);
      Serial.println(" to HIGH");
    }
    digitalWrite(pumpPin, HIGH);
    if (DEBUG_MODE) {
      Serial.print("DEBUG: Pin ");
      Serial.print(pumpPin);
      Serial.print(" state = ");
      Serial.println(digitalRead(pumpPin));
    }
  } else {
    if (DEBUG_MODE) {
      Serial.println("DEBUG: ERROR - pumpPin not set!");
    }
  }
}

// Stop post-run timer
void stopPostRunTimer() {
  if (!postRunTimerActive) {
    return;
  }
  
  postRunTimerActive = false;
  postRunTimerStartTime = 0;
  postRunTimerEndTime = 0;
  
  // Deactivate pump
  if (pumpPin >= 0) {
    digitalWrite(pumpPin, LOW);
    if (DEBUG_MODE) {
      Serial.print("DEBUG: Pump deactivated on pin ");
      Serial.println(pumpPin);
    }
  }
  
  // Stop PWM output (set to 0%)
  setOutputPWM(PWM_OUTPUT_FREQ_POSTRUN, 0);
}

// Update post-run timer status
void updatePostRunTimer() {
  if (!postRunTimerActive) {
    return;
  }
  
  unsigned long now = millis();
  
  // Check if post-run timer has expired
  if (now >= postRunTimerEndTime) {
    stopPostRunTimer();
  }
}

// Get post-run timer status
bool isPostRunTimerActive() {
  return postRunTimerActive;
}

// Get remaining post-run timer time in seconds
unsigned long getPostRunTimerRemainingSeconds() {
  if (!postRunTimerActive) {
    return 0;
  }
  
  unsigned long now = millis();
  if (now >= postRunTimerEndTime) {
    return 0;
  }
  
  return (postRunTimerEndTime - now) / 1000;
}

// Get post-run timer remaining time formatted as string
String getPostRunTimerTimeString() {
  if (!postRunTimerActive) {
    return "";
  }
  
  unsigned long seconds = getPostRunTimerRemainingSeconds();
  unsigned long minutes = seconds / 60;
  unsigned long secs = seconds % 60;
  
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%lu.%lu", minutes, secs / 6);
  return String(buffer);
}

// Set pump control pin
void setPostRunTimerPumpPin(int pin) {
  pumpPin = pin;
  if (pumpPin >= 0) {
    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW);  // Start with pump OFF
  }
}
