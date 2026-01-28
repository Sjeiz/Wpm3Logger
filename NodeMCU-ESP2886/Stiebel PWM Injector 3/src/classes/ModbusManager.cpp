#include "ModbusManager.h"
#include "../globals.h"   // for networkManager
#include "../helpers.h"   // for logMessage

ModbusManager::ModbusManager() : modbusClient(nullptr), isgStatus(ISG_MODBUS_READ_ERROR), initialized(false) {}

void ModbusManager::begin(const String& hostOrIp, uint16_t port) {
    if (modbusClient) {
        delete modbusClient;
        modbusClient = nullptr;
        busy = false;
    }
    IPAddress ip;
    if (ip.fromString(hostOrIp)) {
        // Direct IP-adres
    } else {
        ip = networkManager.resolveHostName(hostOrIp.c_str());
        if (ip == IPAddress(0,0,0,0)) {
            initialized = false;
            logMessage(String("[ERROR] Hostname resolve failed: ") + hostOrIp, LogLevel::LOG_NORMAL);
            return;
        }
    }
    modbusClient = new ModbusClientTCPasync(ip, port);
    modbusClient->onDataHandler([this](ModbusMessage response, uint32_t token) {
        this->handleModbusData(response, token);
    });
    modbusClient->onErrorHandler([this](Error error, uint32_t token) {
        this->handleModbusError(error, token);
    });
    modbusClient->setTimeout(10000);
    modbusClient->setIdleTimeout(60000);
    initialized = true;
}

void ModbusManager::poll() {
    if (modbusClient && !busy) {
        Error err = modbusClient->addRequest(millis(), ISG_SLAVE_ID, READ_INPUT_REGISTER, ISG_OPERATING_STATUS_ADDR, 1);
        if (err == SUCCESS) {
            busy = true;
        }
        // Error handling/logging can be done here
    }
}

void ModbusManager::setOnStatusUpdate(void (*callback)(uint16_t)) {
    statusUpdateCallback = callback;
}

void ModbusManager::loop() {
    const unsigned long POLL_INTERVAL_MS = 1000; // evt. uit config.h halen
    unsigned long now = millis();
    if (!busy && modbusClient && (now - lastPoll >= POLL_INTERVAL_MS)) {
        poll();
        lastPoll = now;
    }
}

uint16_t ModbusManager::getStatus() const {
    return isgStatus;
}

void ModbusManager::handleModbusData(ModbusMessage response, uint32_t token) {
    busy = false;
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
    busy = false;
    isgStatus = ISG_MODBUS_READ_ERROR;
    if (statusUpdateCallback) statusUpdateCallback(isgStatus);
}
