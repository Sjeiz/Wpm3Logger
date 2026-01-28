#include "TelnetLogger.h"
extern "C" {
#include "../classes/StatusInfo.h"
}

void TelnetLogger::logStatus(const StatusInfo& statusInfo) {
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
TelnetLogger::TelnetLogger(WiFiServer* server) : _server(server) {}
void TelnetLogger::log(const String& msg) {
    if (_client && _client.connected()) {
        _client.print(msg);
    }
}
void TelnetLogger::handleClient() {
    if (_server->hasClient()) {
        if (!_client || !_client.connected()) {
            _client = _server->accept();
        } else {
            WiFiClient newClient = _server->accept();
            newClient.stop();
        }
    }
    if (_client && _client.connected() && _client.available()) {
        _client.readStringUntil('\n'); // discard input
    }
}
