#include "ModbusManager.h"

ModbusManager::ModbusManager() : modbusClient(nullptr), isgStatus(ISG_MODBUS_READ_ERROR) {}

void ModbusManager::begin(IPAddress isg_ip, uint16_t port) {
    modbusClient = new ModbusClientTCPasync(isg_ip, port);
    modbusClient->onDataHandler([this](ModbusMessage response, uint32_t token) {
        this->handleModbusData(response, token);
    });
    modbusClient->onErrorHandler([this](Error error, uint32_t token) {
        this->handleModbusError(error, token);
    });
    modbusClient->setTimeout(10000);
    modbusClient->setIdleTimeout(60000);
// ...existing code...
}

void ModbusManager::poll() {
    if (modbusClient) {
        Error err = modbusClient->addRequest(millis(), ISG_SLAVE_ID, READ_INPUT_REGISTER, ISG_OPERATING_STATUS_ADDR, 1);
        // Error handling/logging kan hier
    }
}

void ModbusManager::setOnStatusUpdate(void (*callback)(uint16_t)) {
    statusUpdateCallback = callback;
}

void ModbusManager::loop() {
    // Hier kan evt. periodiek pollen
}

uint16_t ModbusManager::getStatus() const {
    return isgStatus;
}

void ModbusManager::handleModbusData(ModbusMessage response, uint32_t token) {
    if (response.getFunctionCode() == 4 && response.size() >= 5) {
        uint8_t dataHi = response[3];
        uint8_t dataLo = response[4];
        isgStatus = (dataHi << 8) | dataLo;
        if (statusUpdateCallback) statusUpdateCallback(isgStatus);
    } else {
        isgStatus = ISG_MODBUS_READ_ERROR;
        if (statusUpdateCallback) statusUpdateCallback(isgStatus);
    }
}

void ModbusManager::handleModbusError(Error error, uint32_t token) {
    isgStatus = ISG_MODBUS_READ_ERROR;
    if (statusUpdateCallback) statusUpdateCallback(isgStatus);
}
