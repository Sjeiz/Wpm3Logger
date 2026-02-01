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

#include "sensors/PwmInSensor.h"
// PWM input sensor
PwmInSensor pwmInSensor(PIN_PWM_IN);


void setup() {
      pwmInSensor.begin();
    // Start Telnet server
    logManager.getTelnetBridge()->begin();
  Serial.println("\n--- Starting Stiebel PWM Injector 3 ---\n");
  
  // Initialize serial, wait for it to settle and add to logging manager
  Serial.begin(115200);
  delay(10);
  logManager.addLogger(&serialLogger);
  logManager.addLogger(&telnetLogger);
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

  // Los ISG_HOST op naar IP-adres en sla op in globale variabele (reboot if failed)
  isgIp = resolveHost(ISG_HOST);
  if (isgIp == IPAddress(0,0,0,0)) {
    logMessage("💥 ISG_HOST DNS-resolutie mislukt, rebooting...");
    unsigned long start = millis();
    while (millis() - start < 10000UL) {
      webServerManager.handleClient();
      ArduinoOTA.handle();
      yield();
      delay(50);
    }
    ESP.restart();
  }

  // Start ModbusManager met IP-adres
  modbusManager.begin(isgIp, ISG_PORT);

  char heapMsg[64];
  snprintf(heapMsg, sizeof(heapMsg), "[INFO] Initialization completed (free heap: %.1f KB)", ESP.getFreeHeap() / 1024.0);
  logMessage(heapMsg);
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

  // Herinitialiseer ModbusManager alleen met eerder opgeloste IP
  if (!modbusManager.isInitialized()) {
    modbusManager.begin(isgIp, ISG_PORT);
  }
  if(modbusManager.isInitialized()) modbusManager.loop(); // Poll Modbus data
  else tryInitModbusManager();

  // 1. Get latest Modbus status value or overridden value from serial console for testing purposes
  uint16_t isgStatus;
  uint16_t isgFlowRate;
  if(!modbusOverrideFlag & modbusManager.isInitialized()) {
    isgStatus = modbusManager.getByName("OPERATING_STATUS");
    isgFlowRate = modbusManager.getByName("FLOW_RATE");
  } else {
    isgStatus = modbusOverrideBits;
    isgFlowRate = 0;
  }


  // 1b. Read input sensors
  float pwmIn = pwmInSensor.read();
  float flowTemp = flowTempSensor.read();

  // 2. State machine update based on Modbus status or test override
  stateManager.update(isgStatus);

  // 3. Update outputs based on state
  outputManager.loop(stateManager.currentStateName());

  // 4. Read back actual output states naar lokale variabelen
  bool pumpBlocked = digitalRead(PIN_PUMP_BLOCKED) == HIGH;
  bool pumpForced  = digitalRead(PIN_PUMP_FORCE) == HIGH;
  int pwmOut      = PWM_OUT_DUTY_PERCENT;

  // 5. Fill StatusInfo with latest data (now reflecting actual outputs)
  StatusInfo statusInfo = updateStatusInfo(isgStatus, flowTemp, isgFlowRate, pwmIn, pwmOut, pumpBlocked, pumpForced);

  // 6. Log status
  logManager.loop(statusInfo);
}
