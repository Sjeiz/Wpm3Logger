#pragma once


#include <Arduino.h>
#include <ModbusClientTCPasync.h>
#include "config.h"

#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

class ModbusManager {

public:
    ModbusManager(const ModbusConfig& cfg);
    ~ModbusManager();

    void begin(const String& hostOrIp, uint16_t port = 502);
    void loop();

    bool isBusy() const;
    bool isInitialized() const;
    uint16_t getByName(const char* name) const;
    uint16_t getByAddress(uint16_t address) const;

private:
    void startAsyncRead(uint8_t index);
    void handleModbusData(ModbusMessage response, uint32_t token);
    void handleModbusError(Error error, uint32_t token);
    void advance();

    const ModbusConfig& cfg;
    ModbusClientTCPasync* modbusClient;

    uint16_t* values;

    bool busy;
    uint32_t busySince;
    uint32_t lastPoll;
    uint8_t currentIndex;

    bool initialized;
};

#endif // MODBUSMANAGER_H