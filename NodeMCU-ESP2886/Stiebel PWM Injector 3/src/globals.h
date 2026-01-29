#pragma once
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

extern LogManager logManager;
extern SerialLogger serialLogger;
extern TelnetLogger telnetLogger;
extern OutputManager outputManager;
extern FlowTempSensor flowTempSensor;
extern NetworkManager networkManager;
extern WebServerManager webServerManager;
extern ModbusManager modbusManager;
extern StateManager stateManager;
extern unsigned long stateEnterTime;