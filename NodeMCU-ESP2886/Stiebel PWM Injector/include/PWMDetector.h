#ifndef PWMDETECTOR_H
#define PWMDETECTOR_H

#include <Arduino.h>
#include "Config.h"

// PWM measurement variables (volatile for interrupt use)
extern volatile unsigned long highDuration;
extern volatile unsigned long lowDuration;
extern volatile unsigned long edgeCount;

// PWM results (updated by interrupt handler)
extern float dutyIn;
extern float freqIn;
extern bool pwmDetected;

// Initialize PWM detection on specified pin
void initPWMDetector(int pwmPin);

// Process PWM measurements in main loop
void updatePWMDetection();

// Get PWM detection state change (returns true if state changed)
bool pwmStateChanged();

#endif
