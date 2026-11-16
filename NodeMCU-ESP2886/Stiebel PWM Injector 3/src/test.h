
#pragma once
#include <Arduino.h>

extern bool isgOverrideFlag;
extern uint16_t isgOverrideBits;

void handleSerialTestCommand(const String& cmd);
void printSerialTestHelp();
uint16_t parseIsgBits(const String& bitNamesStr);
uint16_t readIsgStatus(uint16_t realBits);
