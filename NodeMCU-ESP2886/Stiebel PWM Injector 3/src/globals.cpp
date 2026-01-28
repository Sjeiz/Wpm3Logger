#include "classes/LogManager.h"
#include "loggers/SerialLogger.h"
#include "loggers/TelnetLogger.h"
#include "classes/WebLogger.h"
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
OutputManager outputManager;
FlowTempSensor flowTempSensor(PIN_FLOW_TEMP);
NetworkManager networkManager;
WebServerManager webServerManager(&networkManager.webLogger);
ModbusManager modbusManager;
StateManager stateManager;
unsigned long stateEnterTime = 0;
