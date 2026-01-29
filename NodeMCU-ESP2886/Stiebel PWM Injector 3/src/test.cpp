
// src/test.cpp
// Serial test interface for the state machine
#include <Arduino.h>
#include "globals.h"
#include "classes/ModbusManager.h"
#include "test.h"

uint16_t modbusOverrideBits = 0;
bool modbusOverrideFlag = false;

struct BitName {
    const char* name;
    uint16_t value;
};

const BitName bitNames[] = {
    {"COMPRESSOR", ISG_STATUS_COMPRESSOR},
    {"DEFROSTING", ISG_STATUS_DEFROSTING},
    {"COOLING", ISG_STATUS_COOLING},
    {"HEATING", ISG_STATUS_HEATING},
    {"HOT_WATER", ISG_STATUS_HOT_WATER},
    {"READ_ERROR", ISG_MODBUS_READ_ERROR},
    {nullptr, 0}
};

void handleSerialTestCommand(const String& line) {
  if (line.startsWith("test")) {
    String bitNamesStr = line.substring(4);
    bitNamesStr.trim();
    if (bitNamesStr.length() == 0) {
      printSerialTestHelp();
    } else if (bitNamesStr.equalsIgnoreCase("off")) {
      modbusOverrideFlag = false;
      Serial.println(F("[TEST] Override disabled."));
    } else {
      modbusOverrideBits = parseModbusBits(bitNamesStr);
      modbusOverrideFlag = true;
      Serial.print(F("[TEST] Override active (0x"));
      Serial.println(String(modbusOverrideBits, HEX) + "): " + bitNamesStr);
    }
  }
}

void printSerialTestHelp() {
  Serial.println(F("[TEST] Test interface commands:"));
  Serial.println(F("[TEST]   test <bitname> [<bitname> ...]  - Override Modbus bits (e.g.: test COMPRESSOR HEATING)"));
  Serial.println(F("[TEST]   test off                        - Disable override"));
  Serial.print(F("[TEST] Bit names: "));
  for (int i = 0; bitNames[i].name; ++i) {
    Serial.print(bitNames[i].name);
    if (bitNames[i+1].name) Serial.print(" ");
  }
  Serial.println();
}

uint16_t parseModbusBits(const String& input) {
    uint16_t bits = 0;
    String s = input;
    s.trim();
    s.toUpperCase();
    unsigned int start = 0;
    while (start < s.length()) {
        int end = s.indexOf(' ', start);
        if (end == -1) end = s.length();
        String token = s.substring(start, end);
        token.trim();
        for (int i = 0; bitNames[i].name; ++i) {
            if (token == bitNames[i].name) {
                bits |= bitNames[i].value;
                break;
            }
        }
        start = end + 1;
    }
    return bits;
}

uint16_t readModbusRegister(uint16_t realBits) {
  if (modbusOverrideFlag) {
    return modbusOverrideBits;
  }
  return realBits;
}
