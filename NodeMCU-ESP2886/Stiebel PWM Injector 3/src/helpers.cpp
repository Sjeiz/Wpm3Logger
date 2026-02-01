
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "globals.h"
#include "config.h"
#include "helpers.h"
#include "test.h"
#include "loggers/Logger.h"

// Decodes Modbus status bitflags to string (HK2_PUMP only as comment)
String decodeModbusStatus(uint16_t status) {
  char hexbuf[7];
  snprintf(hexbuf, sizeof(hexbuf), "0x%04X", status);
  String result = "Modbus:(";
  result += hexbuf;
  result += ") ";
  String flags = "";
  int realFlagCount = 0;
  // --- PUMPS ---
  if (status & (1 << 0)) { flags += "HK1, "; realFlagCount++; }
  // if (status & (1 << 1)) flags += "HK2_PUMP, "; // B1 (ignored, connected differently)
  // --- PROGRAMS / MODES ---
  if (status & (1 << 2)) { flags += "HEATUP, "; realFlagCount++; }
  if (status & (1 << 3)) { flags += "NHZ, "; realFlagCount++; }
  if (status & (1 << 4)) { flags += "HEAT, "; realFlagCount++; }
  if (status & (1 << 5)) { flags += "WATER, "; realFlagCount++; }
  if (status & (1 << 6)) { flags += "COMPR, "; realFlagCount++; }
  if (status & (1 << 7)) { flags += "SUMMER, "; realFlagCount++; }
  if (status & (1 << 8)) { flags += "COOL, "; realFlagCount++; }
  if (status & (1 << 9)) { flags += "DEFR, "; realFlagCount++; }
  // --- SILENT MODES ---
  if (status & (1 << 10)) { flags += "SILENT1, "; realFlagCount++; }
  if (status & (1 << 11)) { flags += "SILENT2, "; realFlagCount++; }
  if (realFlagCount > 0) {
    flags = flags.substring(0, flags.length() - 2); // remove last ", "
    result += flags;
  } else {
    result += "IDLE";
  }
  return result;
}

// Moving average buffer voor flow rate
#define FLOWRATE_MA_SIZE 2
static uint16_t flowRateBuffer[FLOWRATE_MA_SIZE] = {0};
static uint8_t flowRateIndex = 0;
static uint8_t flowRateCount = 0;

StatusInfo updateStatusInfo(uint16_t isgStatus, float flowTemp, uint16_t flowRate, float pwmIn, int pwmOut, bool pumpBlocked, bool pumpForced) {
  StatusInfo info;
  // State
  snprintf(info.stateName, sizeof(info.stateName), "%s", stateManager.currentStateName());
  unsigned long elapsedMs = millis() - stateEnterTime;
  snprintf(info.stateTimeStr, sizeof(info.stateTimeStr), "%s", elapsedTimeToString(elapsedMs).c_str());

  // PumpHK2 state
  if (pumpBlocked) {
    snprintf(info.outputStatus, sizeof(info.outputStatus), "BLOCKED");
  } else if (pumpForced) {
    snprintf(info.outputStatus, sizeof(info.outputStatus), "FORCED");
  } else {
    snprintf(info.outputStatus, sizeof(info.outputStatus), "NORMAL");
  }

  // Compressor state
  snprintf(info.compressorStr, sizeof(info.compressorStr), "%s", (isgStatus & ISG_STATUS_COMPRESSOR) ? "ON" : "OFF");

  // PWM-out: show OFF if 0%, else show percentage (in POST_RUN always show 25%)
  if (strcmp(stateManager.currentStateName(), "POST_RUN") == 0) {
    if (PWM_OUT_DUTY_PERCENT == 0) {
      snprintf(info.pwmOutVal, sizeof(info.pwmOutVal), "OFF");
    } else {
      snprintf(info.pwmOutVal, sizeof(info.pwmOutVal), "%d%%", PWM_OUT_DUTY_PERCENT);
    }
  } else {
    if (pwmOut == 0) {
      snprintf(info.pwmOutVal, sizeof(info.pwmOutVal), "OFF");
    } else {
      snprintf(info.pwmOutVal, sizeof(info.pwmOutVal), "%d%%", pwmOut);
    }
  }

  // PWM-in
  snprintf(info.pwmInVal, sizeof(info.pwmInVal), "%.0f%%", pwmIn);

  // Flow temp
  info.flowTemp = flowTemp;
  // Flow rate moving average, ignore error values (ISG_MODBUS_READ_ERROR)
  if (flowRate != ISG_MODBUS_READ_ERROR) {
    flowRateBuffer[flowRateIndex] = flowRate;
    flowRateIndex = (flowRateIndex + 1) % FLOWRATE_MA_SIZE;
    if (flowRateCount < FLOWRATE_MA_SIZE) flowRateCount++;
  }
  uint32_t flowRateSum = 0;
  uint8_t flowRateValid = 0;
  for (uint8_t i = 0; i < flowRateCount; i++) {
    if (flowRateBuffer[i] != ISG_MODBUS_READ_ERROR) {
      flowRateSum += flowRateBuffer[i];
      flowRateValid++;
    }
  }
  info.flowRate = (flowRateValid > 0) ? (flowRateSum / flowRateValid) : flowRate;

  // Wifi state
  info.wifiOk = networkManager.isWiFiConnected();
  snprintf(info.modbusStr, sizeof(info.modbusStr), "%s", evaluateIsgStatus(isgStatus));
  return info;
}

  // Flow temp
  const char* formatStatusLogLine(const StatusInfo& statusInfo) {
  char flowTempStr[16];
  if ((int)statusInfo.flowTemp == ISG_MODBUS_READ_ERROR) {
    snprintf(flowTempStr, sizeof(flowTempStr), "FAIL");
  } else {
    snprintf(flowTempStr, sizeof(flowTempStr), "%.1f°C", statusInfo.flowTemp);
  }
  
  // Flow rate
  char flowRateStr[16];
  if ((int)statusInfo.flowRate == ISG_MODBUS_READ_ERROR) {
    snprintf(flowRateStr, sizeof(flowRateStr), "FAIL");
  } else {
    snprintf(flowRateStr, sizeof(flowRateStr), "%.1f l/min", statusInfo.flowRate / 100.0f);
  }
  
  // compose status line
  static char buf[400];
  // Haal modbus status als hex uit statusInfo.modbusStr ("0x1234" of "FAIL")
  uint16_t modbusVal = 0;
  if (strncmp(statusInfo.modbusStr, "0x", 2) == 0) {
    modbusVal = (uint16_t)strtol(statusInfo.modbusStr + 2, nullptr, 16);
  }
  String modbusDecoded = decodeModbusStatus(modbusVal);
  snprintf(buf, sizeof(buf),
    "State: %s (%s)  PumpHK2:%s  Compressor:%s  PWM-out:%s  PWM-in:%s  Flow:%s @ %s  WiFi:%s  %s",
    statusInfo.stateName,
    statusInfo.stateTimeStr,
    statusInfo.outputStatus,
    statusInfo.compressorStr,
    statusInfo.pwmOutVal,
    statusInfo.pwmInVal,
    flowTempStr,
    flowRateStr,
    statusInfo.wifiOk ? "OK" : "FAIL",
    modbusDecoded.c_str()
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