#pragma once
#include <Arduino.h>

struct StatusInfo {
    char stateName[16];
    char outputStatus[8];
    char compressorStr[4];
    char pwmOutVal[8];
    char pwmInVal[8];
    float flowTemp;
    uint16_t flowRate;
    bool wifiOk;
    char modbusStr[24];
    char stateTimeStr[16];
};