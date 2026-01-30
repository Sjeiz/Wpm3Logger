


#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "config.h"

#include "classes/StatusInfo.h"

// Formatteert een StatusInfo tot een logregel (zonder newline, geen heap-allocatie)
const char* formatStatusLogLine(const StatusInfo& statusInfo);

String timeStamp(time_t t = 0);
// Overload: logMessage(message) gebruikt standaard LOG_NORMAL
void logMessage(const String& message);

// DNS-resolutie helper: retourneert IP-adres of IPAddress(0,0,0,0) bij falen
IPAddress resolveHost(const char* host, int maxTries = 6, int retryDelayMs = 10000);

const char* outputStatusName(const char* stateName);
void handleOutputState(const char* stateName, const uint16_t status);
void logMessage(const String& message, const LogLevel level);
void handleSerialTestInput();
String elapsedTimeToString(unsigned long elapsedMs);
void tryInitModbusManager();
const char* evaluateIsgStatus(uint16_t isgStatus);
