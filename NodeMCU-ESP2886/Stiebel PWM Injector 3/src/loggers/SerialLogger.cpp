#include "SerialLogger.h"
#include <Arduino.h>

void SerialLogger::log(const String& msg) {
    Serial.print(msg);
}

void SerialLogger::logStatus(const StatusInfo& statusInfo) {
    char buf[220];
    snprintf(buf, sizeof(buf), "State:%s%s  PumpHK2:%s  Compressor:%s  PWM-out:%s  FlowTemp:%.1f  WiFi:%s  Modbus:%s\n",
        statusInfo.stateName.c_str(),
        statusInfo.stateTimeStr.c_str(),
        statusInfo.outputStatus.c_str(),
        statusInfo.compressorStr.c_str(),
        statusInfo.pwmOutVal.c_str(),
        statusInfo.flowTemp,
        statusInfo.wifiOk ? "OK" : "FAIL",
        statusInfo.modbusStr.c_str()
    );
    log(String("[INFO] ") + buf);
}
