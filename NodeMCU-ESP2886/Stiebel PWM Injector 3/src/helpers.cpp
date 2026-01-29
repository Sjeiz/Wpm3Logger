
// --- Includes ---
#include <Arduino.h>
#include "helpers.h"
#include "globals.h"
#include "config.h"
#include "test.h"
#include "loggers/Logger.h"
#include <time.h>

// Overload: logMessage(message) gebruikt standaard LOG_NORMAL
void logMessage(const String& message) {
  logMessage(message, LogLevel::LOG_NORMAL);
}

// Tries to initialize ModbusManager if WiFi is connected and logs the result
void tryInitModbusManager() {
  if (networkManager.isWiFiConnected()) {
    modbusManager.begin(ISG_HOST, ISG_MODBUS_PORT);
    if (modbusManager.isInitialized()) {
      logMessage("[INFO] ModbusManager initialized (WiFi connected)");
    } else {
      logMessage("[WARN] ModbusManager init failed (host unresolved)");
    }
  }
}
// Geeft een string voor de modbus status ("FAIL" of hex string)
const char* evaluateIsgStatus(uint16_t isgStatus) {
  static char hexbuf[12];
  if (isgStatus == ISG_MODBUS_READ_ERROR) {
    return "FAIL";
  } else {
    snprintf(hexbuf, sizeof(hexbuf), "0x%04X", isgStatus);
    return hexbuf;
  }
}
// Zet elapsedMs om naar een string als "xx s", "x.x min" of "h:mm hr"
String elapsedTimeToString(unsigned long elapsedMs) {
  if (elapsedMs < 60000UL) {
    return String(elapsedMs / 1000UL) + " s";
  } else if (elapsedMs < 3600000UL) {
    // Toon 1 decimaal, maar zonder dtostrf
    float min = elapsedMs / 60000.0f;
    return String(min, 1) + " min";
  } else {
    unsigned long totalMin = elapsedMs / 60000UL;
    unsigned long hours = totalMin / 60;
    unsigned long mins = totalMin % 60;
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu:%02lu hr", hours, mins);
    return String(buf);
  }
}


const char* outputStatusName(const char* stateName) {
  if (strcmp(stateName, "DEFROST") == 0) return "BLOCKED";
  if (strcmp(stateName, "POST_RUN") == 0) return "FORCED";
  return "NORMAL";
}


// ...handleOutputState is nu verplaatst naar OutputManager::loop(stateName)...

void logMessage(const String& message, const LogLevel level) {
  // LogLevel minLevel = LogLevel::LOG_NORMAL;
  // if (DEBUG) minLevel = LogLevel::LOG_DEBUG;
  // else if (VERBOSE) minLevel = LogLevel::LOG_VERBOSE;
  // if (static_cast<int>(level) > static_cast<int>(minLevel)) return;
  // time_t now = time(nullptr);
  // char buf[20];
  // strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
  // String logLine = "[" + String(buf) + "] ";
  // logLine += message + "\r\n";
  // //logManager.log(logLine);
  Serial.print(message);
}

void handleSerialTestInput() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleSerialTestCommand(line);
  }
}
