#pragma once
#include <Arduino.h>

struct StatusInfo {
    String stateName;
    String outputStatus;
    String compressorStr;
    String pwmOutVal;
    float flowTemp;
    float flowRate = 0.0f;
    bool wifiOk;
    String modbusStr;
    String stateTimeStr;
};