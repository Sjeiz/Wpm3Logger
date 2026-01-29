// Eenvoudige Telnet <-> UART bridge
#include "TelnetBridge.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

TelnetBridge::TelnetBridge(WiFiServer* server) : _server(server) {}

void TelnetBridge::handleClient() {
    if (_server->hasClient()) {
        if (!_client || !_client.connected()) {
            _client = _server->accept();
        } else {
            WiFiClient newClient = _server->accept();
            newClient.stop();
        }
    }
    // UART → Telnet
    while (Serial.available() && _client && _client.connected()) {
        _client.write(Serial.read());
    }
    // Telnet → UART
    while (_client && _client.connected() && _client.available()) {
        Serial.write(_client.read());
    }
}
