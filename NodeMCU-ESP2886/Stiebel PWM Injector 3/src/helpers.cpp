#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "globals.h"
#include "config.h"
#include "helpers.h"
#include "test.h"
#include "loggers/Logger.h"

StatusInfo updateStatusInfo(uint16_t isgStatus, uint16_t isgFlowRate, float pwmIn, int pwmOut, bool pumpBlocked, bool pumpForced) {
  StatusInfo info;
  // State
  snprintf(info.stateName, sizeof(info.stateName), "%s", stateManager.currentStateName());

  // PumpHK2
  if (pumpBlocked) {
    snprintf(info.outputStatus, sizeof(info.outputStatus), "BLOCKED");
  } else if (pumpForced) {
    snprintf(info.outputStatus, sizeof(info.outputStatus), "FORCED");
  } else {
    snprintf(info.outputStatus, sizeof(info.outputStatus), "NORMAL");
  }
  snprintf(info.compressorStr, sizeof(info.compressorStr), "%s", (isgStatus & ISG_STATUS_COMPRESSOR) ? "ON" : "OFF");
  if (pwmOut > 10) {
    int percent = (int)((pwmOut / 1023.0f) * 100.0f + 0.5f);
    snprintf(info.pwmOutVal, sizeof(info.pwmOutVal), "%d%%", percent);
  } else {
    snprintf(info.pwmOutVal, sizeof(info.pwmOutVal), "OFF");
  }
  snprintf(info.pwmInVal, sizeof(info.pwmInVal), "%.0f%%", pwmIn);
  info.flowTemp = flowTempSensor.read();
  info.flowRate = isgFlowRate;
  info.wifiOk = networkManager.isWiFiConnected();
  snprintf(info.modbusStr, sizeof(info.modbusStr), "%s", evaluateIsgStatus(isgStatus));
  unsigned long elapsedMs = millis() - stateEnterTime;
  snprintf(info.stateTimeStr, sizeof(info.stateTimeStr), "%s", elapsedTimeToString(elapsedMs).c_str());
  return info;
}


// Formatteert een StatusInfo tot een logregel (zonder newline), zonder heap-allocatie
const char* formatStatusLogLine(const StatusInfo& statusInfo) {
  char flowTempStr[16];
  if ((int)statusInfo.flowTemp == ISG_MODBUS_READ_ERROR) {
    snprintf(flowTempStr, sizeof(flowTempStr), "FAIL");
  } else {
    snprintf(flowTempStr, sizeof(flowTempStr), "%.1f°C", statusInfo.flowTemp);
  }
  
  char flowRateStr[16];
  if ((int)statusInfo.flowRate == ISG_MODBUS_READ_ERROR) {
    snprintf(flowRateStr, sizeof(flowRateStr), "FAIL");
  } else {
    snprintf(flowRateStr, sizeof(flowRateStr), "%.1f l/min", statusInfo.flowRate / 100.0f);
  }
  
  static char buf[360];
  snprintf(buf, sizeof(buf),
    "State: %s (%s)  PumpHK2:%s  Compressor:%s  PWM-out:%s  PWM-in:%s  Flow:%s @ %s  WiFi:%s  Modbus:%s",
    statusInfo.stateName,
    statusInfo.stateTimeStr,
    statusInfo.outputStatus,
    statusInfo.compressorStr,
    statusInfo.pwmOutVal,
    statusInfo.pwmInVal,
    flowTempStr,
    flowRateStr,
    statusInfo.wifiOk ? "OK" : "FAIL",
    statusInfo.modbusStr
  );
  return buf;
}


IPAddress resolveHost(const char* host, int maxTries, int retryDelayMs) {
  IPAddress outIp(0,0,0,0);
  for (int tryCount = 0; tryCount < maxTries; ++tryCount) {
    logMessage(String("🔎 ISG_HOST DNS-resolutie poging ") + (tryCount+1) + "/" + maxTries + ": " + host);
    if (WiFi.hostByName(host, outIp) == 1 && outIp != IPAddress(0,0,0,0)) {
      logMessage(String("✅ ISG_HOST resolved: ") + outIp.toString());
      return outIp;
    } else {
      logMessage("❌ ISG_HOST niet gevonden, opnieuw proberen over 10s...");
      unsigned long start = millis();
      while (millis() - start < (unsigned long)retryDelayMs) {
        webServerManager.handleClient();
        ArduinoOTA.handle();
        yield();
        delay(50);
      }
    }
  }
  logMessage("💥 ISG_HOST DNS-resolutie mislukt");
  return IPAddress(0,0,0,0);
}

void logMessage(const String& message) {
  logMessage(message, LogLevel::LOG_NORMAL);
}

// Tries to initialize ModbusManager if WiFi is connected and logs the result
void tryInitModbusManager() {
  if (networkManager.isWiFiConnected()) {
    modbusManager.begin(isgIp, ISG_PORT);
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
     return String(elapsedMs / 1000UL) + "s";
  } else if (elapsedMs < 3600000UL) {
    // Toon 1 decimaal, maar zonder dtostrf
    float min = elapsedMs / 60000.0f;
     return String(min, 1) + "min";
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
  // Stuur alle loglevels via logManager zodat alles (ook debug) op serial, telnet en web komt
  logManager.log(message, level);
}

void handleSerialTestInput() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleSerialTestCommand(line);
  }
}

String timeStamp(time_t t) {
  if (t == 0) t = time(nullptr);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
  return String(buf);
}