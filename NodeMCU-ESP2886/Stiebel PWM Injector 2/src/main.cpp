// Always include headers first!
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ModbusTCP.h>
#include <time.h>
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Globale ModbusTCP client
ModbusTCP mb;


// Lees het ISG operating status register via ModbusTCP (emelianov)
uint16_t getISGOperatingStatus() {
  IPAddress isg_ip;
  if (!WiFi.hostByName(ISG_HOST, isg_ip)) {
    return ISG_MODBUS_READ_ERROR;
  }
  if (!mb.isConnected(isg_ip)) {
    mb.connect(isg_ip, ISG_MODBUS_PORT);
    if (!mb.isConnected(isg_ip)) {
      return ISG_MODBUS_READ_ERROR;
    }
  }
  uint16_t value = 0;
  bool result = mb.readHreg(ISG_SLAVE_ID, ISG_OPERATING_STATUS_ADDR, &value, 1);
  if (result) {
    return value;
  } else {
    return ISG_MODBUS_READ_ERROR;
  }
}



void logMessage(const String& message, LogLevel level = LOG_NORMAL);
// (logCombinedStatus/logOperatingStatusFlags: not used)
float readFlowTemp();
const char* outputStatusName(State state);
State evaluateState(uint16_t status, bool wifiOk);
void handleOutputState(State newState, uint16_t status);
// void logOperatingStatusFlags(uint16_t status); // not used
void syncTimeWithNTP();
void tryConnectWiFi();
// int myFunction(int x, int y); // not used


float readPWMIn() { return 75.0; }
String readPWMOut() { return "OFF"; }

// Temperature sensor on D2 (PIN_FLOW_TEMP)
OneWire oneWire(PIN_FLOW_TEMP);
DallasTemperature sensors(&oneWire);
float lastFlowTemp = NAN;
unsigned long lastTempRead = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 5000;

float readFlowTemp() {
  unsigned long now = millis();
  if (now - lastTempRead > TEMP_READ_INTERVAL_MS || isnan(lastFlowTemp)) {
    sensors.requestTemperatures();
    lastFlowTemp = sensors.getTempCByIndex(0);
    lastTempRead = now;
  }
  return lastFlowTemp;
}

const char* outputStatusName(State state) {
  switch (state) {
    case DEFROST: return "BLOCKED";
    case POST_RUN: return "FORCED";
    default: return "NORMAL";
  }
}

void logMessage(const String& message, LogLevel level) {
  // Show everything from the chosen minimum log level and lower (so DEBUG also shows VERBOSE and NORMAL)
  int minLevel = LOG_NORMAL;
  if (DEBUG) minLevel = LOG_DEBUG;
  else if (VERBOSE) minLevel = LOG_VERBOSE;
  if (level > minLevel) return;
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
  if (level == LOG_VERBOSE) Serial.print("[VERBOSE] ");
  else if (level == LOG_DEBUG) Serial.print("[DEBUG] ");
  Serial.println(message);
}


// OutputState currentOutputState = ERROR; // not used
// State currentState = ERROR; // not used
unsigned long postRunStart = 0;
const unsigned long POST_RUN_DURATION_MS = POST_RUN_DURATION_MIN * 60000UL;

// State machine tracking
static State previousState = ERROR;
static State currentState = ERROR;
// bool lastHotWater = false; // not used


State evaluateState(uint16_t status, bool wifiOk) {
  if (!wifiOk || status == ISG_MODBUS_READ_ERROR) return ERROR;
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

void handleOutputState(State newState, uint16_t status) {
  static bool lastWasHotWater = false;

  // Set hardware outputs directly based on newState
  switch (newState) {
    case DEFROST:
      digitalWrite(PIN_PUMP_ON, LOW);
      digitalWrite(PIN_PUMP_BLOCKED, HIGH);
      analogWrite(PIN_PWM_OUT, 0);
      break;
    case POST_RUN:
      digitalWrite(PIN_PUMP_ON, HIGH);
      digitalWrite(PIN_PUMP_BLOCKED, LOW);
      analogWrite(PIN_PWM_OUT, (int)(PWM_OUT_DUTY_PERCENT * 1023 / 100));
      break;
    default:
      digitalWrite(PIN_PUMP_ON, LOW);
      digitalWrite(PIN_PUMP_BLOCKED, LOW);
      analogWrite(PIN_PWM_OUT, 0);
      break;
  }
  lastWasHotWater = (newState == HOT_WATER);
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
  sensors.begin();
  // Initialize PWM output
  pinMode(PIN_PWM_OUT, OUTPUT);
  analogWriteFreq(PWM_OUT_FREQUENCY_HZ);
  int pwmValue = (int)(PWM_OUT_DUTY_PERCENT * 1023 / 100);
  analogWrite(PIN_PWM_OUT, pwmValue);
  logMessage("Setup started");
  tryConnectWiFi();
  lastNTPSync = millis();
  
  logMessage("Setup complete");

  // Initialize currentState after WiFi/Modbus setup
  bool modbusOk = wifiConnected; // Simuleer: Modbus OK als WiFi OK
  if (wifiConnected && modbusOk) {
    currentState = STANDBY;
  } else {
    currentState = ERROR;
  }
}

void loop() {
  // 0. State tracking: Store current state before updating
  previousState = currentState;

  // 1. Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      logMessage("\xE2\x9D\x8C WiFi connection lost!");
      wifiConnected = false;
      lastWiFiAttempt = millis();
    }
    if (millis() - lastWiFiAttempt >= (unsigned long)WIFI_RETRY_SEC * 1000UL) {
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

  // 2. Resync NTP every hour if WiFi is connected
  if (wifiConnected && millis() - lastNTPSync >= NTP_RESYNC_INTERVAL_MS) {
    syncTimeWithNTP();
    lastNTPSync = millis();
  }

  // 3. Read inputs
  // a. ISG_OPERATING_STATUS uitlezen via Modbus
  uint16_t isgStatus = getISGOperatingStatus();
  // b. Read PWM-in via GPIO (dummy function)
  float pwmInVal = readPWMIn();
  // c. Read FlowTemp via GPIO
  float flowTempVal = readFlowTemp();

  // 4. Determine state and transitions
  currentState = evaluateState(isgStatus, wifiConnected);

  // 4b. POST_RUN logic and transitions
  static unsigned long postRunStart = 0;
  static bool lastWasHotWater = false;
  // POST_RUN: HOT_WATER -> STANDBY
  State outputState = currentState;
  if (lastWasHotWater && currentState == STANDBY) {
    outputState = POST_RUN;
    postRunStart = millis();
    logMessage("Output state: POST_RUN (forced)");
  }
  // Cancel POST_RUN if status is no longer STANDBY
  if (previousState == POST_RUN && currentState != STANDBY) {
    // outputState remains as determined
    logMessage("Output state: POST_RUN cancelled");
  }
  // End POST_RUN after duration
  if (outputState == POST_RUN && millis() - postRunStart > POST_RUN_DURATION_MS) {
    outputState = STANDBY;
    logMessage("Output state: POST_RUN ended");
  }
  lastWasHotWater = (currentState == HOT_WATER);

  // 5. Set outputs
  handleOutputState(outputState, isgStatus);

  // 6. Log status (with current values)
  static State previousLoggedState = ERROR;
  String pwmOutVal = readPWMOut();
  if (currentState != previousLoggedState || VERBOSE) {
    char buf[128];
    snprintf(buf, sizeof(buf), "State:%s  Output:%s  PWM-in:%.1f%%  PWM-out:%s  Flow:%.1f°C  WiFi:%s  Modbus:%s",
      currentState == ERROR ? "ERROR" :
      currentState == STANDBY ? "STANDBY" :
      currentState == DEFROST ? "DEFROST" :
      currentState == COOLING ? "COOLING" :
      currentState == HOT_WATER ? "HOT_WATER" :
      currentState == HEATING ? "HEATING" :
      currentState == POST_RUN ? "POST_RUN" : "UNKNOWN",
      outputStatusName(currentState),
      pwmInVal,
      pwmOutVal.c_str(),
      flowTempVal,
      wifiConnected ? "OK" : "FAIL",
      (isgStatus == ISG_MODBUS_READ_ERROR ? "FAIL" : "OK")
    );
    logMessage(buf);
    previousLoggedState = currentState;
  }

  delay(ISG_POLL_INTERVAL_SEC * 1000UL);
}

// Put function definitions here:
// int myFunction(int x, int y) { return x + y; } // not used