#pragma once
#include <Arduino.h>

uint16_t parseModbusBits(const String& input);
void printSerialTestHelp();
void handleSerialTestCommand(const String& line);
uint16_t readModbusRegister(uint16_t realBits);
