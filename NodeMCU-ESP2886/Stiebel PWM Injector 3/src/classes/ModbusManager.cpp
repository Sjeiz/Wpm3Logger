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

void ModbusManager::begin(IPAddress ip, uint16_t port) {
    if (modbusClient) {
        delete modbusClient;
        modbusClient = nullptr;
        busy = false;
    }
    if (!ip) {
        initialized = false;
        logMessage(String("[ERROR] Ongeldig IP-adres voor ModbusManager: ") + ip.toString(), LogLevel::LOG_DEBUG);
        return;
    }
    logMessage("[DEBUG] Creating ModbusClientTCPasync for " + ip.toString() + ":" + String(port), LogLevel::LOG_DEBUG);
    modbusClient = new ModbusClientTCPasync(ip, port);
    modbusClient->onDataHandler([this](ModbusMessage response, uint32_t token) {
        this->handleModbusData(response, token);
    });
    modbusClient->onErrorHandler([this](Error error, uint32_t token) {
        logMessage("[DEBUG] ModbusManager onErrorHandler called for token: " + String(token) + " error: " + String((uint8_t)error), LogLevel::LOG_DEBUG);
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
            //logMessage("[DEBUG] ModbusManager getByName: " + String(name) + " found value: " + String(values[i]), LogLevel::LOG_DEBUG);
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
    modbusClient->addRequest(
        index,               // transaction ID
        ISG_SLAVE_ID,        // unit ID = 1
        READ_INPUT_REGISTER, // function code
        addr,                // register address
        1                    // count
    );
    //logMessage("[DEBUG] ModbusManager starting async read of register: " + String(cfg.regs[index].name) + " (addr " + String(addr) + ")", LogLevel::LOG_DEBUG);
}

void ModbusManager::handleModbusData(ModbusMessage response, uint32_t token) {
    //logMessage("[DEBUG] ModbusManager received response for token: " + String(token), LogLevel::LOG_DEBUG);
    if (token < cfg.count) {
        uint16_t val;
        uint8_t dataHi = response[3];
        uint8_t dataLo = response[4];
        val = (dataHi << 8) | dataLo;
        //logMessage("[DEBUG] ModbusManager received data for register: " + String(cfg.regs[token].name) + " value: " + String(val) + " dataHi:" + String(dataHi) + " dataLo:" + String(dataLo), LogLevel::LOG_DEBUG);
        values[token] = val;
    }

    busy = false;
    advance();
}

void ModbusManager::handleModbusError(Error error, uint32_t token) {
    if (token < cfg.count) {
        values[token] = ISG_MODBUS_READ_ERROR;
    }

    // Herinitialiseer bij error 224 (TCP timeout/disconnect)
    if ((uint8_t)error == 224) {
        initialized = false;
        logMessage("[WARN] ModbusManager: error 224 gedetecteerd, client wordt opnieuw geïnitialiseerd");
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


