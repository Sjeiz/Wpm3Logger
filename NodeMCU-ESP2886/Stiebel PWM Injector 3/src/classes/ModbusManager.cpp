#include "ModbusManager.h"
#include "../config.h"
#include "../helpers.h"
#include "../globals.h"


ModbusManager::ModbusManager(const ModbusConfig& cfg)
    : cfg(cfg),
      modbusClient(nullptr),
      busy(false),
      busySince(0),
      lastPoll(0),
      currentIndex(0),
      initialized(false)
{
    values = new uint16_t[cfg.count];
    for (uint8_t i = 0; i < cfg.count; i++) {
        values[i] = ISG_MODBUS_READ_ERROR;
    }
}


ModbusManager::~ModbusManager() {
    if (values) delete[] values;
}

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
    modbusClient->setTimeout(500);
    modbusClient->setIdleTimeout(60000);
    initialized = true;
}


void ModbusManager::loop() {
    // check if modbus client is initialized
    if (!initialized) return;
    
    uint32_t now = millis();

    // Fail-safe busy timeout
    if (busy && (now - busySince > 500)) {
        busy = false;
    }

    // No polling if busy
    if (busy) return;

    // Check if it's time to poll
    if (now - lastPoll < ISG_POLL_INTERVAL_SEC * 1000UL) return;
    
    // Start new polling cycle
    if (currentIndex == 0) {
        lastPoll = now;
    }
    startAsyncRead(currentIndex);
}


bool ModbusManager::isBusy() const {
    return busy;
}

bool ModbusManager::isInitialized() const {
    return initialized;
}


uint16_t ModbusManager::getByName(const char* name) const {
    for (uint8_t i = 0; i < cfg.count; i++) {
        if (strcmp(cfg.regs[i].name, name) == 0) {
            return values[i];
        }
    }
    return ISG_MODBUS_READ_ERROR;
}

uint16_t ModbusManager::getByAddress(uint16_t address) const {
    for (uint8_t i = 0; i < cfg.count; i++) {
        if (cfg.regs[i].address == address) {
            return values[i];
        }
    }
    return ISG_MODBUS_READ_ERROR;
}


void ModbusManager::startAsyncRead(uint8_t index) {
    busy = true;
    busySince = millis();

    uint16_t addr = cfg.regs[index].address;

    // transactionId = index zodat we weten welk register het is
    modbusClient->addRequest(index, READ_HOLD_REGISTER, addr, 1);
}

void ModbusManager::handleModbusData(ModbusMessage response, uint32_t token) {
    if (token < cfg.count) {
        uint16_t val;
        response.get(3, val);   // offset 3 = eerste register
        values[token] = val;
    }

    busy = false;
    advance();
}

void ModbusManager::handleModbusError(Error error, uint32_t token) {
    if (token < cfg.count) {
        values[token] = ISG_MODBUS_READ_ERROR;
    }

    busy = false;
    advance();
}

void ModbusManager::advance() {
    currentIndex++;
    if (currentIndex >= cfg.count) {
        currentIndex = 0;
    }
}


