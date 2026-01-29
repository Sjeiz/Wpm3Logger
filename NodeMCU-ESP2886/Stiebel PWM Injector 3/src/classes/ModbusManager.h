#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <Arduino.h>
#include <ModbusClientTCPasync.h>
#include "config.h"

class ModbusManager {
public:
    ModbusManager();
    void begin(const String& hostOrIp, uint16_t port);
    void poll();
    void setOnStatusUpdate(void (*callback)(uint16_t));
    void loop();
    uint16_t readInputRegister(uint16_t address) const;
    bool isBusy() const { return busy; }
    bool isInitialized() const { return initialized; }

private:
    ModbusClientTCPasync* modbusClient;
    uint16_t isgStatus;
    void (*statusUpdateCallback)(uint16_t) = nullptr;
    bool busy = false;
    bool initialized = false;
    unsigned long lastPoll = 0;
    void handleModbusData(ModbusMessage response, uint32_t token);
    void handleModbusError(Error error, uint32_t token);
    static void onDataThunk(ModbusMessage response, uint32_t token, void* arg);
    static void onErrorThunk(Error error, uint32_t token, void* arg);
};

#endif // MODBUSMANAGER_H
