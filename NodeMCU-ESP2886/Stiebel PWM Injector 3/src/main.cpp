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
  Serial.println("\n--- Starting Stiebel PWM Injector 3 ---\n");
  
  // Initialize serial, wait for it to settle and add to logging manager
  Serial.begin(115200);
  delay(10);
  logManager.addLogger(&serialLogger);
  logMessage("[INFO] Initialization started");
  
  // Initialize timezone
  configTime(TIMEZONE, NTP_SERVER);

  // Initialize network and related services (WiFi, OTA, NTP)
  networkManager.begin();

  // Initialize other managers, services, sensors
  ArduinoOTA.begin();                              // Enable Over-the-Air (OTA) firmware updates
  outputManager.begin();                           // Initialize output manager  
  webServerManager.setup();                        // Initialize web server for status/logging
  logManager.addLogger(logManager.getWebLogger()); // Initialize Web logger
  stateManager.begin(errorState);                  // Initialize state machine, starting in error state
  flowTempSensor.begin();                          // Initialize flow temperature sensor

  logMessage("[INFO] Initialization completed");
}


void loop() {
  // Handle network tasks (WiFi connectivity, OTA, NTP)
  networkManager.loop();

  // Handle OTA updates
  ArduinoOTA.handle(); 

  // Handle inputs and clients
  handleSerialTestInput(); // Can override Modbus status bits for testing purposes
  logManager.getTelnetBridge()->handleClient();
  webServerManager.handleClient();
  
  // Only poll Modbus when ModbusManager is initialized
  if(modbusManager.isInitialized()) modbusManager.loop(); // Poll Modbus data
  else tryInitModbusManager();

  // Get latest Modbus status value or overridden value from serial console for testing purposes
  uint16_t isgStatus;
  if(!modbusOverrideFlag & modbusManager.isInitialized())
    isgStatus = modbusManager.readInputRegister(2500);
  else
    isgStatus = modbusOverrideBits;

  // State machine update based on Modbus status or test override
  stateManager.update(isgStatus);

  // Fill StatusInfo with latest data
  StatusInfo statusInfo;
  statusInfo.stateName     = stateManager.currentStateName();
  statusInfo.outputStatus  = outputStatusName(statusInfo.stateName.c_str());
  statusInfo.compressorStr = (isgStatus & ISG_STATUS_COMPRESSOR) ? "ON" : "OFF";
  statusInfo.pwmOutVal     = (stateManager.getCurrentStatePtr() == postRunState) ? String(PWM_OUT_DUTY_PERCENT) + "%" : "OFF";
  statusInfo.flowTemp      = flowTempSensor.read();
  statusInfo.wifiOk        = networkManager.isWiFiConnected();
  statusInfo.modbusStr     = evaluateIsgStatus(isgStatus);
  unsigned long elapsedMs  = millis() - stateEnterTime;
  statusInfo.stateTimeStr  = elapsedTimeToString(elapsedMs);

  outputManager.loop(statusInfo.stateName.c_str());
  logManager.loop(statusInfo);
}
