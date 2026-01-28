// --- Includes ---
#include "globals.h"
#include "config.h"
#include "helpers.h"
#include "test.h"
#include "loggers/Logger.h"
#include "classes/StatusInfo.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <ModbusClientTCPasync.h>
#include <DallasTemperature.h>
#include <OneWire.h>



void setup() {
  // Initialize timezone
  setenv("TZ", TIMEZONE, 1); tzset();

  // Initialize serial, wait for it to settle and add to logging manager
  Serial.begin(115200);
  delay(10);
  logManager.addLogger(&serialLogger);

  logMessage("[INFO] Initialization started", LogLevel::LOG_NORMAL);

  // Initialize network and related services (WiFi, OTA, TelnetLogger, WebLogger)
  networkManager.begin();
  
  // Add network based loggers to logging manager
  logManager.addLogger(logManager.getTelnetLogger());
  logManager.addLogger(logManager.getWebLogger());

  // Initialize GPIOs
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_FLOW_TEMP, INPUT_PULLUP);

  // Initialize other managers and services
  ArduinoOTA.begin();             // Enable Over-the-Air (OTA) firmware updates
  outputManager.begin();          // Initialize output manager  
  webServerManager.setup();       // Start web server for status/logging
  stateManager.begin(errorState); // state machine initialization in error state
  flowTempSensor.begin();         // Initialize flow temperature sensor

  logMessage("[INFO] Initialization completed", LogLevel::LOG_NORMAL);
}

void loop() {
  // Handle network tasks
  networkManager.loop();

  // Handle serial test commands
  handleSerialTestInput();
  
  // Only perform modbus tasks when ModbusManager is initialized
  if(modbusManager.isInitialized()) {
    // Poll Modbus data
    modbusManager.loop();
    uint16_t isgStatus = readModbusRegister(modbusManager.getStatus()); // Get latest Modbus status

    // State machine update, deze zou ook zonder modbusmanager kunnen werken
    stateManager.update(isgStatus); // Update state machine with current Modbus status
    
    State* currentStatePtr = stateManager.getCurrentStatePtr();
    static State* lastStatePtr = nullptr;
    if (currentStatePtr != lastStatePtr) {
      stateEnterTime = millis();
      lastStatePtr = currentStatePtr;
      handleOutputState(stateManager.currentStateName(), isgStatus);
    }

    logManager.getTelnetLogger()->handleClient();
    
    // StatusInfo vullen en logManager.loop() aanroepen
    StatusInfo statusInfo;
    statusInfo.stateName = stateManager.currentStateName();
    statusInfo.outputStatus = outputStatusName(statusInfo.stateName.c_str());
    statusInfo.compressorStr = (isgStatus & ISG_STATUS_COMPRESSOR) ? "ON" : "OFF";
    statusInfo.pwmOutVal = (strcmp(statusInfo.stateName.c_str(), "POST_RUN") == 0) ? String(PWM_OUT_DUTY_PERCENT) + "%" : "OFF";
    statusInfo.flowTemp = flowTempSensor.read();
    statusInfo.wifiOk = networkManager.isWiFiConnected();
    if (isgStatus == ISG_MODBUS_READ_ERROR) {
      statusInfo.modbusStr = "FAIL";
    } else {
      char hexbuf[12];
      snprintf(hexbuf, sizeof(hexbuf), "0x%04X", isgStatus);
      statusInfo.modbusStr = hexbuf;
    }
    unsigned long elapsedMs = millis() - stateEnterTime;
    char stateTimeStr[24] = "";
    if (elapsedMs < 60000UL) {
      snprintf(stateTimeStr, sizeof(stateTimeStr), " (%lus)", elapsedMs / 1000UL);
    } else if (elapsedMs < 3600000UL) {
      snprintf(stateTimeStr, sizeof(stateTimeStr), " (%.1f min)", elapsedMs / 60000.0f);
    } else {
      unsigned long totalMin = elapsedMs / 60000UL;
      unsigned long hours = totalMin / 60;
      unsigned long mins = totalMin % 60;
      snprintf(stateTimeStr, sizeof(stateTimeStr), " (%lu:%02lu)", hours, mins);
    }
    statusInfo.stateTimeStr = stateTimeStr;
    logManager.loop(statusInfo);
    webServerManager.handleClient();
    ArduinoOTA.handle();
  } else if (networkManager.isWiFiConnected()) {
    // Initialize ModbusManager once WiFi is connected
    modbusManager.begin(ISG_HOST, ISG_MODBUS_PORT);
    if (modbusManager.isInitialized()) {
      logMessage("[INFO] ModbusManager initialized (WiFi connected)", LogLevel::LOG_NORMAL);
    } else {
      logMessage("[WARN] ModbusManager init failed (host unresolved)", LogLevel::LOG_NORMAL);
    }
  }
}
