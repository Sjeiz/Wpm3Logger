
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "config.h"

enum LogLevel { LOG_NORMAL, LOG_VERBOSE, LOG_DEBUG };
enum PumpState {
  ERROR,
  STANDBY,
  DEFROST,
  COOLING,
  HOT_WATER,
  HEATING,
  POST_RUN
};

void logMessage(const String& message, LogLevel level = LOG_NORMAL);
float readPWMIn();
String readPWMOut();
float readFlowTemp();
const char* pumpStatusName(PumpState state);
void logCombinedStatus(PumpState state);
void setPumpOutputs(PumpState state);
PumpState evaluatePumpState(uint16_t status, bool wifiOk, bool modbusOk);
void handlePumpState(uint16_t status, bool wifiOk, bool modbusOk);
void logOperatingStatusFlags(uint16_t status);
void syncTimeWithNTP();
void tryConnectWiFi();
int myFunction(int x, int y);

float readPWMIn() { return 75.0; }
String readPWMOut() { return "OFF"; }
float readFlowTemp() { return 48.0; }

const char* pumpStatusName(PumpState state) {
  switch (state) {
    case DEFROST: return "BLOCKED";
    case POST_RUN: return "FORCED";
    default: return "NORMAL";
  }
}

void logMessage(const String& message, LogLevel level) {
  if (level == LOG_DEBUG && !DEBUG) return;
  if (level == LOG_VERBOSE && !VERBOSE) return;
  time_t now = time(nullptr);
  char buf[20];
  if (now > 100000) {
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
  } else {
    strcpy(buf, "1970-01-01 00:00:00");
  }
  Serial.print("[");
  Serial.print(buf);
  Serial.print("] ");
  Serial.println(message);
}

void logCombinedStatus(PumpState state) {
  char buf[128];
  snprintf(buf, sizeof(buf), "State:%s  PumpHK2:%s  PWM-in:%.1f%%  PWM-out:%s  Flow:%.1f°C",
    state == ERROR ? "ERROR" :
    state == STANDBY ? "STANDBY" :
    state == DEFROST ? "DEFROST" :
    state == COOLING ? "COOLING" :
    state == HOT_WATER ? "HOT_WATER" :
    state == HEATING ? "HEATING" :
    state == POST_RUN ? "POST_RUN" : "UNKNOWN",
    pumpStatusName(state),
    readPWMIn(),
    readPWMOut().c_str(),
    readFlowTemp()
  );
  logMessage(buf);
}

PumpState currentPumpState = ERROR;
unsigned long postRunStart = 0;
const unsigned long POST_RUN_DURATION_MS = POST_RUN_DURATION_MIN * 60000UL;
bool lastHotWater = false;

void setPumpOutputs(PumpState state) {
  switch (state) {
    case ERROR:
    case STANDBY:
    case COOLING:
    case HOT_WATER:
    case HEATING:
      digitalWrite(PIN_PUMP_ON, LOW);
      digitalWrite(PIN_PUMP_BLOCKED, LOW);
      break;
    case DEFROST:
      digitalWrite(PIN_PUMP_ON, LOW);
      digitalWrite(PIN_PUMP_BLOCKED, HIGH);
      break;
    case POST_RUN:
      digitalWrite(PIN_PUMP_ON, HIGH);
      digitalWrite(PIN_PUMP_BLOCKED, LOW);
      break;
  }
}

PumpState evaluatePumpState(uint16_t status, bool wifiOk, bool modbusOk) {
  if (!wifiOk || !modbusOk) return ERROR;
  bool compressor = status & ISG_STATUS_COMPRESSOR;
  bool defrost    = status & ISG_STATUS_DEFROSTING;
  bool cooling    = status & ISG_STATUS_COOLING;
  bool hotwater   = status & ISG_STATUS_HOT_WATER;
  bool heating    = status & ISG_STATUS_HEATING;

  if (!compressor) return STANDBY;
  if (defrost)     return DEFROST;
  if (cooling)     return COOLING;
  if (hotwater)    return HOT_WATER;
  if (heating)     return HEATING;
  return STANDBY;
}

void handlePumpState(uint16_t status, bool wifiOk, bool modbusOk) {
  static PumpState lastState = ERROR;
  static bool lastWasHotWater = false;
  PumpState newState = evaluatePumpState(status, wifiOk, modbusOk);

  // POST_RUN: HOT_WATER -> STANDBY
  if (lastWasHotWater && newState == STANDBY) {
    newState = POST_RUN;
    postRunStart = millis();
    logMessage("Pump state: POST_RUN (forced)");
  }
  // Cancel POST_RUN if status is no longer STANDBY
  if (lastState == POST_RUN && newState != STANDBY) {
    newState = evaluatePumpState(status, wifiOk, modbusOk);
    logMessage("Pump state: POST_RUN cancelled");
  }
  // End POST_RUN after duration
  if (newState == POST_RUN && millis() - postRunStart > POST_RUN_DURATION_MS) {
    newState = STANDBY;
    logMessage("Pump state: POST_RUN ended");
  }

  // Log state change
  if (newState != lastState) {
    const char* stateNames[] = {"ERROR", "STANDBY", "DEFROST", "COOLING", "HOT_WATER", "HEATING", "POST_RUN"};
    logMessage(String("Pump state: ") + stateNames[newState]);
  }
  setPumpOutputs(newState);
  lastState = newState;
  lastWasHotWater = (evaluatePumpState(status, wifiOk, modbusOk) == HOT_WATER);
}

void logOperatingStatusFlags(uint16_t status) {
  String prefix = "                         ";
  String flags = prefix + "Active flags:";
  bool any = false;
  if (status & ISG_STATUS_HK1_PUMP)           { flags += " HK1_PUMP"; any = true; }
  if (status & ISG_STATUS_HK2_PUMP)           { flags += " HK2_PUMP"; any = true; }
  if (status & ISG_STATUS_HEAT_UP_PROGRAM)    { flags += " HEAT-UP_PROGRAM"; any = true; }
  if (status & ISG_STATUS_NHZ_STAGES_RUNNING) { flags += " NHZ_STAGES_RUNNING"; any = true; }
  if (status & ISG_STATUS_HEATING)            { flags += " HEATING"; any = true; }
  if (status & ISG_STATUS_HOT_WATER)          { flags += " HOT_WATER"; any = true; }
  if (status & ISG_STATUS_COMPRESSOR)         { flags += " COMPRESSOR"; any = true; }
  if (status & ISG_STATUS_SUMMER_MODE_ACTIVE) { flags += " SUMMER_MODE_ACTIVE"; any = true; }
  if (status & ISG_STATUS_COOLING)            { flags += " COOLING"; any = true; }
  if (status & ISG_STATUS_DEFROSTING)         { flags += " DEFROSTING"; any = true; }
  if (status & ISG_STATUS_SILENT_MODE_1)      { flags += " SILENT_MODE_1"; any = true; }
  if (status & ISG_STATUS_SILENT_MODE_2)      { flags += " SILENT_MODE_2"; any = true; }
  if (!any) flags += " (none)";
  logMessage(flags);
}

unsigned long lastWiFiAttempt = 0;
bool wifiConnected = false;
unsigned long lastNTPSync = 0;
const unsigned long NTP_RESYNC_INTERVAL_MS = 3600000UL; // 1 hour

void syncTimeWithNTP() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  logMessage("Synchronizing time with NTP...");
  time_t now = time(nullptr);
  int retries = 10;
  while (now < 8 * 3600 * 2 && retries > 0) {
    delay(500);
    now = time(nullptr);
    retries--;
  }
  if (now > 8 * 3600 * 2) {
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    String msg = "\xE2\x9C\x85 NTP time: ";
    msg += buf;
    logMessage(msg);
  } else {
    logMessage("\xE2\x9D\x8C NTP sync failed!");
  }
}

void tryConnectWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  Serial.print(" ");
  int wifiTimeout = WIFI_TIMEOUT_SEC * 2; // WIFI_TIMEOUT_SEC x 2 x 500ms = seconds
  while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0) {
    delay(500);
    Serial.print(".");
    wifiTimeout--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    String msg = "\xE2\x9C\x85 Connection established! IP address: ";
    msg += WiFi.localIP().toString();
    logMessage(msg);
    wifiConnected = true;
    syncTimeWithNTP();
  } else {
    logMessage("\xE2\x9D\x8C WiFi connection failed!");
    wifiConnected = false;
  }
  lastWiFiAttempt = millis();
}

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(PIN_PUMP_ON, OUTPUT);
  pinMode(PIN_PUMP_BLOCKED, OUTPUT);
  logMessage("Setup started");
  tryConnectWiFi();
  lastNTPSync = millis();
  int result = myFunction(2, 3);
  logMessage("Setup complete");
}

void loop() {
  // Check WiFi connection and retry if needed
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      logMessage("\xE2\x9D\x8C WiFi connection lost!");
      wifiConnected = false;
      lastWiFiAttempt = millis();
    }
    if (millis() - lastWiFiAttempt >= (unsigned long)WIFI_RETRY_SEC * 1000UL) { // retry interval from config
      tryConnectWiFi();
    }
  } else {
    if (!wifiConnected) {
      String msg = "\xE2\x9C\x85 Connection re-established! IP address: ";
      msg += WiFi.localIP().toString();
      logMessage(msg);
      wifiConnected = true;
    }
  }
  // Resync NTP every hour if WiFi is connected
  if (wifiConnected && millis() - lastNTPSync >= NTP_RESYNC_INTERVAL_MS) {
    syncTimeWithNTP();
    lastNTPSync = millis();
  }
  logMessage("Loop running");

  // Example verbose polling message:
  logMessage("Polling ISG_OPERATING_STATUS (address " + String(ISG_OPERATING_STATUS_ADDR) + ") from " + String(ISG_HOST) + ":" + String(ISG_MODBUS_PORT) + "...", LOG_VERBOSE);
  // Stel voorbeeldwaarde in voor demonstratie (vervang door echte Modbus-uitlezing)
  uint16_t exampleStatus = ISG_STATUS_HK1_PUMP | ISG_STATUS_COMPRESSOR | ISG_STATUS_HEATING;
  logMessage("✅ ISG_OPERATING_STATUS = " + String(exampleStatus));
  logOperatingStatusFlags(exampleStatus);
  handlePumpState(exampleStatus, true, true); // Simuleer WiFi/Modbus OK voor demo
  // Log gecombineerde statusregel
  static PumpState lastLoggedState = ERROR;
  PumpState currentState = evaluatePumpState(exampleStatus, true, true);
  if (currentState != lastLoggedState) {
    logCombinedStatus(currentState);
    lastLoggedState = currentState;
  }
  delay(2000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}