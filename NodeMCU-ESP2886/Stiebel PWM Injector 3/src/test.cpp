
// src/test.cpp
// Serial test interface for the state machine
#include <Arduino.h>
#include "globals.h"
#include "classes/IsgApiManager.h"
#include "test.h"
#include "helpers.h"

uint16_t isgOverrideBits = 0;
bool isgOverrideFlag = false;

struct BitName {
    const char* name;
    uint16_t value;
};

const BitName bitNames[] = {
    {"COMPRESSOR", ISG_471_COMPRESSOR},   // 471 bit 6 COMPRESSOR (VERDICHTER)
    {"DEFROST", ISG_471_DEFROST},         // 471 bit 9 DEFROST (ABTAUBETRIEB)
    {"COOLING_MODE", ISG_471_COOLING_MODE}, // 471 bit 8 COOLING MODE (KUEHLBETRIEB)
    {"HEATING", ISG_471_HEATING},         // 471 bit 4 HEATING (HEIZBETRIEB)
    {"DHW_MODE", ISG_471_DHW_MODE},       // 471 bit 5 DHW MODE (WARMWASSERBETRIEB)
    {"READ_ERROR", ISG_READ_ERROR},       // no value for web ID 471
    {nullptr, 0}
};

void handleSerialTestCommand(const String& line) {
  logMessage(String("[DEBUG] handleSerialTestCommand triggered: [") + line + "]");
  String asciiCodes = "[DEBUG] ASCII codes: ";
  for (unsigned int i = 0; i < line.length(); ++i) {
    asciiCodes += String((int)line[i]);
    if (i < line.length() - 1) asciiCodes += ",";
  }
  logMessage(asciiCodes);
  logMessage(String("[DEBUG] line.length() = ") + line.length());
  logMessage(String("[DEBUG] line.startsWith('test') = ") + (line.startsWith("test") ? "true" : "false"));
  if (line.startsWith("test")) {
    String bitNamesStr = line.substring(4);
    bitNamesStr.trim();
    if (bitNamesStr.length() == 0) {
      logMessage("[DEBUG] test: print help");
      printSerialTestHelp();
    } else if (bitNamesStr.equalsIgnoreCase("off")) {
      isgOverrideFlag = false;
      logMessage("[TEST] Override disabled.");
    } else {
      isgOverrideBits = parseIsgBits(bitNamesStr);
      isgOverrideFlag = true;
      logMessage(String("[TEST] Override active (0x") + String(isgOverrideBits, HEX) + "): " + bitNamesStr);
    }
  } else {
    logMessage(String("[DEBUG] handleSerialTestCommand: onbekend commando [") + line + "]");
  }
}

void printSerialTestHelp() {
  logMessage(F("[TEST] Test interface commands:"));
  logMessage(F("[TEST]   test <bitname> [<bitname> ...]  - Override web ID 471 bits (e.g.: test COMPRESSOR HEATING)"));
  logMessage(F("[TEST]   test off                        - Disable override"));
  String bitNamesStr = F("[TEST] Bit names: ");
  for (int i = 0; bitNames[i].name; ++i) {
    bitNamesStr += bitNames[i].name;
    if (bitNames[i+1].name) bitNamesStr += " ";
  }
  logMessage(bitNamesStr);
}

uint16_t parseIsgBits(const String& input) {
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

uint16_t readIsgStatus(uint16_t realBits) {
  if (isgOverrideFlag) {
    return isgOverrideBits;
  }
  return realBits;
}
