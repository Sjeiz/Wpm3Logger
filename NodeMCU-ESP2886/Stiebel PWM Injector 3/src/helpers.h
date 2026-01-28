// Overload: logMessage(message) gebruikt standaard LOG_NORMAL
void logMessage(const String& message);
#pragma once

#include <Arduino.h>
#include "config.h"

const char* outputStatusName(const char* stateName);
void handleOutputState(const char* stateName, const uint16_t status);
void logMessage(const String& message, const LogLevel level);
void handleSerialTestInput();
String elapsedTimeToString(unsigned long elapsedMs);
void tryInitModbusManager();
const char* evaluateIsgStatus(uint16_t isgStatus);
