#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <Arduino.h>
#include <ModbusClientTCPasync.h>
#include "config.h"

class ModbusManager {
public:
    ModbusManager();
    void begin(IPAddress isg_ip, uint16_t port);
    void poll();
    void setOnStatusUpdate(void (*callback)(uint16_t));
    void loop();
    uint16_t getStatus() const;

private:
    ModbusClientTCPasync* modbusClient;
    uint16_t isgStatus;
    void (*statusUpdateCallback)(uint16_t) = nullptr;
    void handleModbusData(ModbusMessage response, uint32_t token);
    void handleModbusError(Error error, uint32_t token);
    static void onDataThunk(ModbusMessage response, uint32_t token, void* arg);
    static void onErrorThunk(Error error, uint32_t token, void* arg);
};

#endif // MODBUSMANAGER_H
