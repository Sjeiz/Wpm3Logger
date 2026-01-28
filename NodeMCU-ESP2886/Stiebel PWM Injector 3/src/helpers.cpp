#include "helpers.h"
#include "globals.h"
#include "test.h"
#include <time.h>

const char* outputStatusName(const char* stateName) {
  if (strcmp(stateName, "DEFROST") == 0) return "BLOCKED";
  if (strcmp(stateName, "POST_RUN") == 0) return "FORCED";
  return "NORMAL";
}


void handleOutputState(const char* stateName, const uint16_t status) {
  if (!stateName) return;
  if (strcmp(stateName, "DEFROST") == 0) {
    outputManager.setDefrost();
  } else if (strcmp(stateName, "POST_RUN") == 0) {
    outputManager.setPostRun(PWM_OUT_DUTY_PERCENT);
  } else {
    outputManager.setNormal();
  }
}

void logMessage(const String& message, const LogLevel level) {
  LogLevel minLevel = LogLevel::LOG_NORMAL;
  if (DEBUG) minLevel = LogLevel::LOG_DEBUG;
  else if (VERBOSE) minLevel = LogLevel::LOG_VERBOSE;
  if (static_cast<int>(level) > static_cast<int>(minLevel)) return;
  time_t now = time(nullptr);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
  String logLine = "[" + String(buf) + "] ";
  logLine += message + "\r\n";
  logManager.log(logLine);
}

void handleSerialTestInput() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleSerialTestCommand(line);
  }
}
