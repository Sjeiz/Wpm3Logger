#include "classes/LogManager.h"
#include "loggers/SerialLogger.h"
#include "loggers/TelnetBridge.h"
#include "loggers/WebLogger.h"
#include "loggers/TelnetLogger.h"
#include "classes/OutputManager.h"
#include "sensors/FlowTempSensor.h"
#include "classes/NetworkManager.h"
#include "classes/WebServerManager.h"
#include "classes/ModbusManager.h"
#include "classes/StateManager.h"
#include <Arduino.h>

// Global object definitions
LogManager logManager;
SerialLogger serialLogger;
TelnetLogger telnetLogger(logManager.getTelnetBridge());
OutputManager outputManager;
FlowTempSensor flowTempSensor(PIN_FLOW_TEMP);
NetworkManager networkManager;
WebServerManager webServerManager(logManager.getWebLogger());
ModbusManager modbusManager(MODBUS_CONFIG);
StateManager stateManager;
unsigned long stateEnterTime = 0;

// Globale inputstatus

#include <ESP8266WiFi.h>
IPAddress isgIp;
