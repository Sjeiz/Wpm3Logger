#pragma once

#include <Arduino.h>
#include "config.h"

const char* outputStatusName(const char* stateName);
void handleOutputState(const char* stateName, const uint16_t status);
void logMessage(const String& message, const LogLevel level);
void handleSerialTestInput();
