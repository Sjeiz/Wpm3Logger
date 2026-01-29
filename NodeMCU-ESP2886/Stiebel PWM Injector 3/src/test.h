
#pragma once
#include <Arduino.h>

extern bool modbusOverrideFlag;
extern uint16_t modbusOverrideBits;

void handleSerialTestCommand(const String& cmd);
void printSerialTestHelp();
uint16_t parseModbusBits(const String& bitNamesStr);
uint16_t readModbusRegister(uint16_t realBits);
