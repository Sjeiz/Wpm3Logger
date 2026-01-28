#pragma once
#include <Arduino.h>

struct StatusInfo {
    String stateName;
    String outputStatus;
    String compressorStr;
    String pwmOutVal;
    float flowTemp;
    bool wifiOk;
    String modbusStr;
    String stateTimeStr;
};