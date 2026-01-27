#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "classes/NetworkManager.h"
#include <ModbusClientTCPasync.h>
#include "classes/ModbusManager.h"
#include <ArduinoOTA.h>
#include <time.h>
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include "classes/WebServerManager.h"
#include "classes/WebLogger.h"
#include "classes/states/State.h"
#include "classes/states/StandbyState.h"
#include "classes/states/ErrorState.h"
// Voeg hier andere states toe indien nodig
// #include "classes/states/HeatingState.h"
// #include "classes/states/DefrostState.h"
// #include "classes/states/HotWaterState.h"
// #include "classes/states/PostRunState.h"

// === GLOBAL CONSTANTS & VARIABLES ===
NetworkManager networkManager;
ModbusManager modbusManager;
OneWire oneWire(PIN_FLOW_TEMP);
DallasTemperature sensors(&oneWire);
float lastFlowTemp = NAN;
unsigned long lastTempRead = 0;
unsigned long postRunStart = 0;
const unsigned long POST_RUN_DURATION_MS = POST_RUN_DURATION_MIN * 60000UL;
State* previousState = nullptr;
State* currentState = nullptr;
WebLogger webLogger(8192); // 8kB buffer
WebServerManager webServerManager(&webLogger);


// === FUNCTION PROTOTYPES ===
void logMessage(const String& message, const LogLevel level = LogLevel::LOG_NORMAL);
float readFlowTemp();
// ...verwijderd: dubbele definities...
State* evaluateState(const uint16_t status);
void handleOutputState(State* newState, const uint16_t status);
// NetworkManager verzorgt WiFi, OTA, NTP, Telnet
float readPWMIn();

float readPWMIn() { return 75.0; }
float readFlowTemp() {
  unsigned long now = millis();
  if (now - lastTempRead > TEMP_READ_INTERVAL_MS || isnan(lastFlowTemp)) {
    sensors.requestTemperatures();
    lastFlowTemp = sensors.getTempCByIndex(0);
    lastTempRead = now;
  }
  return lastFlowTemp;
}

const char* outputStatusName(State* state) {
  if (!state) return "UNKNOWN";
  const char* n = state->name();
  if (strcmp(n, "DEFROST") == 0) return "BLOCKED";
  if (strcmp(n, "POST_RUN") == 0) return "FORCED";
  return "NORMAL";
}

void logMessage(const String& message, const LogLevel level) {
  LogLevel minLevel = LogLevel::LOG_NORMAL;
  if (DEBUG) minLevel = LogLevel::LOG_DEBUG;
  else if (VERBOSE) minLevel = LogLevel::LOG_VERBOSE;
  if (static_cast<int>(level) > static_cast<int>(minLevel)) return;
  time_t now = time(nullptr);
  char buf[20];
  if (now > 100000) {
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
  } else {
    strcpy(buf, "1970-01-01 00:00:00");
  }
  String logLine = "[" + String(buf) + "] ";
  if (level == LogLevel::LOG_VERBOSE) logLine += "[VERBOSE] ";
  else if (level == LogLevel::LOG_DEBUG) logLine += "[DEBUG] ";
  logLine += message + "\r\n";
  Serial.print(logLine);
  webLogger.log(logLine);
  if (webLogger.getLogHtml().length() > 8000) {
    // buffer management is handled in WebLogger
  }
  networkManager.logToTelnet(logLine);
}

State* evaluateState(const uint16_t status) {
  // Hier hoort de state machine logica, placeholder:
  // Return een nieuwe StandbyState als voorbeeld
  return new StandbyState();
}

void handleOutputState(State* newState, const uint16_t status) {
  if (!newState) return;
  const char* n = newState->name();
  if (strcmp(n, "DEFROST") == 0) {
    digitalWrite(PIN_PUMP_ON, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, HIGH);
    analogWrite(PIN_PWM_OUT, 0);
  } else if (strcmp(n, "POST_RUN") == 0) {
    digitalWrite(PIN_PUMP_ON, HIGH);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, (int)(PWM_OUT_DUTY_PERCENT * 1023 / 100));
  } else {
    digitalWrite(PIN_PUMP_ON, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, 0);
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_PUMP_ON, OUTPUT);
  pinMode(PIN_PUMP_BLOCKED, OUTPUT);
  // Activeer interne pull-up op OneWire pin (indien ondersteund)
  pinMode(PIN_FLOW_TEMP, INPUT_PULLUP);
  sensors.begin();
  // Initialize PWM output
  pinMode(PIN_PWM_OUT, OUTPUT);
  analogWriteFreq(PWM_OUT_FREQUENCY_HZ);
  int pwmValue = (int)(PWM_OUT_DUTY_PERCENT * 1023 / 100);
  analogWrite(PIN_PWM_OUT, pwmValue);
  networkManager.begin();
  webServerManager.setup();
  ArduinoOTA.begin();
  logMessage("Setup complete", LogLevel::LOG_NORMAL);
  // Telnet wordt nu door networkManager beheerd
  // ModbusManager initialisatie (na WiFi)
  if (networkManager.isWiFiConnected()) {
    IPAddress isg_ip;
    WiFi.hostByName(ISG_HOST, isg_ip);
    modbusManager.begin(isg_ip, ISG_MODBUS_PORT);
    currentState = new StandbyState();
  } else {
    currentState = new ErrorState();
  }
  // Initialize currentState na WiFi/Modbus setup
  if (networkManager.isWiFiConnected()) {
    IPAddress isg_ip;
    WiFi.hostByName(ISG_HOST, isg_ip);
    modbusManager.begin(isg_ip, ISG_MODBUS_PORT);
    currentState = new StandbyState();
  } else {
    currentState = new ErrorState();
  }
}

void loop() {
  networkManager.loop();
  previousState = currentState;

  // Debug: begin loop
  logMessage("[DEBUG] loop() start", LogLevel::LOG_DEBUG);

  static unsigned long postRunStart = 0;
  State* newState = evaluateState(modbusManager.getStatus());
  logMessage("[DEBUG] evaluateState() klaar", LogLevel::LOG_DEBUG);

  // POST_RUN: HOT_WATER -> STANDBY
  // Hier moet de state machine transitie logica komen
  currentState = newState;
  logMessage(String("[DEBUG] currentState: ") + (currentState ? currentState->name() : "nullptr"), LogLevel::LOG_DEBUG);

  // 5. Set outputs
  handleOutputState(currentState, modbusManager.getStatus());
  logMessage("[DEBUG] handleOutputState() klaar", LogLevel::LOG_DEBUG);

  // 6. Log status (with current values)
  static unsigned long lastLogTime = 0;
  String pwmOutVal = (currentState && strcmp(currentState->name(), "POST_RUN") == 0) ? String(PWM_OUT_DUTY_PERCENT) + "%" : "OFF";
  if (millis() - lastLogTime >= ISG_POLL_INTERVAL_SEC * 1000UL) {
    char buf[200];
    String flowStr = "?";
    String modbusStr;
    uint16_t isgStatus = modbusManager.getStatus();
    if (isgStatus == ISG_MODBUS_READ_ERROR) {
      modbusStr = "FAIL";
    } else {
      char hexbuf[12];
      snprintf(hexbuf, sizeof(hexbuf), "0x%04X", isgStatus);
      modbusStr = hexbuf;
    }
    snprintf(buf, sizeof(buf), "State:%s  Output:%s  PWM-out:%s  WiFi:%s  Modbus:%s",
      currentState ? currentState->name() : "UNKNOWN",
      outputStatusName(currentState),
      pwmOutVal.c_str(),
      networkManager.isWiFiConnected() ? "OK" : "FAIL",
      modbusStr.c_str()
    );
    logMessage(buf);
    lastLogTime = millis();
  }
  webServerManager.handleClient();
  ArduinoOTA.handle();
  logMessage("[DEBUG] loop() end", LogLevel::LOG_DEBUG);
}

// End of main.cpp