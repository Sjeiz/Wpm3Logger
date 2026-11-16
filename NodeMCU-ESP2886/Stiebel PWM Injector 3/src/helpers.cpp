
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "globals.h"
#include "config.h"
#include "helpers.h"
#include "test.h"
#include "loggers/Logger.h"

// Decodes web ID 471 OPERATING STATUS (BETRIEBSSTATUS) to short log labels; bit names in config.h.
String decodeIsgStatus(uint16_t status) {
  char hexbuf[7];
  snprintf(hexbuf, sizeof(hexbuf), "0x%04X", status);
  String result = "ISG471:(";
  result += hexbuf;
  result += ") ";
  String flags = "";
  int realFlagCount = 0;
  // --- PUMPS ---
  if (status & ISG_471_HEATING_CIRCUIT_1_PUMP) { flags += "HK1, "; realFlagCount++; }      // bit 0 HEATING CIRCUIT 1 PUMP
  // bit 1 HEATING CIRCUIT 2 PUMP not logged: same as 402 bit 5 / 485 bit 1, the empty MKP terminal;
  // the floor pump is on HKP (402 bit 4 / 485 bit 0)
  // --- PROGRAMS / MODES ---
  if (status & ISG_471_HEAT_UP_PROGRAM) { flags += "HEATUP, "; realFlagCount++; }          // bit 2 HEAT-UP PROGRAM
  if (status & ISG_471_ELECTRIC_EMERGENCY_BOOSTER_HEATER) { flags += "NHZ, "; realFlagCount++; } // bit 3 ELECTRIC EMERGENCY/BOOSTER HEATER
  if (status & ISG_471_HEATING) { flags += "HEAT, "; realFlagCount++; }                    // bit 4 HEATING
  if (status & ISG_471_DHW_MODE) { flags += "WATER, "; realFlagCount++; }                  // bit 5 DHW MODE
  if (status & ISG_471_COMPRESSOR) { flags += "COMPR, "; realFlagCount++; }                // bit 6 COMPRESSOR
  if (status & ISG_471_SUMMER_MODE) { flags += "SUMMER, "; realFlagCount++; }              // bit 7 SUMMER MODE
  if (status & ISG_471_COOLING_MODE) { flags += "COOL, "; realFlagCount++; }               // bit 8 COOLING MODE
  if (status & ISG_471_DEFROST) { flags += "DEFR, "; realFlagCount++; }                    // bit 9 DEFROST
  // --- SILENT MODES ---
  if (status & ISG_471_SILENT_MODE_1) { flags += "SILENT1, "; realFlagCount++; }           // bit 10 SILENT MODE 1
  if (status & ISG_471_SILENT_MODE_2) { flags += "SILENT2, "; realFlagCount++; }           // bit 11 SILENT MODE 2
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

  // Compressor state: web ID 471 bit 6 COMPRESSOR (VERDICHTER)
  snprintf(info.compressorStr, sizeof(info.compressorStr), "%s", (isgStatus & ISG_471_COMPRESSOR) ? "ON" : "OFF");

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
  // Flow rate moving average, ignore error values (ISG_READ_ERROR)
  if (flowRate != ISG_READ_ERROR) {
    flowRateBuffer[flowRateIndex] = flowRate;
    flowRateIndex = (flowRateIndex + 1) % FLOWRATE_MA_SIZE;
    if (flowRateCount < FLOWRATE_MA_SIZE) flowRateCount++;
  }
  uint32_t flowRateSum = 0;
  uint8_t flowRateValid = 0;
  for (uint8_t i = 0; i < flowRateCount; i++) {
    if (flowRateBuffer[i] != ISG_READ_ERROR) {
      flowRateSum += flowRateBuffer[i];
      flowRateValid++;
    }
  }
  info.flowRate = (flowRateValid > 0) ? (flowRateSum / flowRateValid) : flowRate;

  // Wifi state
  info.wifiOk = networkManager.isWiFiConnected();
  snprintf(info.isgStr, sizeof(info.isgStr), "%s", evaluateIsgStatus(isgStatus));
  return info;
}

  // Flow temp
  const char* formatStatusLogLine(const StatusInfo& statusInfo) {
  char flowTempStr[16];
  if ((int)statusInfo.flowTemp == ISG_READ_ERROR) {
    snprintf(flowTempStr, sizeof(flowTempStr), "FAIL");
  } else {
    snprintf(flowTempStr, sizeof(flowTempStr), "%.1f°C", statusInfo.flowTemp);
  }
  
  // Flow rate
  char flowRateStr[16];
  if ((int)statusInfo.flowRate == ISG_READ_ERROR) {
    snprintf(flowRateStr, sizeof(flowRateStr), "FAIL");
  } else {
    snprintf(flowRateStr, sizeof(flowRateStr), "%.1f l/min", statusInfo.flowRate / 100.0f);
  }
  
  // compose status line
  static char buf[400];
  // Haal de ISG-status (web ID 471) als hex uit statusInfo.isgStr ("0x1234" of "FAIL")
  uint16_t isgVal = 0;
  if (strncmp(statusInfo.isgStr, "0x", 2) == 0) {
    isgVal = (uint16_t)strtol(statusInfo.isgStr + 2, nullptr, 16);
  }
  String isgDecoded = decodeIsgStatus(isgVal);
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
    isgDecoded.c_str()
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

// Tries to initialize IsgApiManager if WiFi is connected and logs the result
void tryInitIsgApiManager() {
  if (networkManager.isWiFiConnected()) {
    isgApiManager.begin(isgIp, ISG_HTTP_PORT);
    if (isgApiManager.isInitialized()) {
      logMessage("[INFO] IsgApiManager initialized (WiFi connected)");
    } else {
      logMessage("[WARN] IsgApiManager init failed (host unresolved)");
    }
  }
}
// Geeft een string voor de ISG-status, web ID 471 ("FAIL" of hex string)
const char* evaluateIsgStatus(uint16_t isgStatus) {
  static char hexbuf[12];
  if (isgStatus == ISG_READ_ERROR) {
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