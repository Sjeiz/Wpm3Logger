#ifndef POSTRUNTIMERMANAGER_H
#define POSTRUNTIMERMANAGER_H

#include <Arduino.h>
#include "Config.h"

// Initialize post-run timer manager with duration parameter
void initPostRunTimer(unsigned long durationMs);

// Get the configured post-run timer duration in milliseconds
unsigned long getPostRunTimerDuration();

// Start post-run timer
void startPostRunTimer();

// Stop post-run timer
void stopPostRunTimer();

// Update post-run timer status (call in main loop)
void updatePostRunTimer();

// Get post-run timer status
bool isPostRunTimerActive();

// Get remaining post-run timer time in seconds
unsigned long getPostRunTimerRemainingSeconds();

// Get post-run timer remaining time formatted as string (e.g., "1.5 min")
String getPostRunTimerTimeString();

// Set pump control pin
void setPostRunTimerPumpPin(int pumpPin);

#endif
