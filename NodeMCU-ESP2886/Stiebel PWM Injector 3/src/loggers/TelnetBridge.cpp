#include "TelnetBridge.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

void TelnetBridge::begin() {
    if (_server) _server->begin();
}

// Eenvoudige Telnet <-> UART bridge
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
    // Telnet → input buffer (parse lines)
    while (_client && _client.connected() && _client.available()) {
        char c = _client.read();
        if (c == '\r') continue; // ignore CR
        if (c == '\n') {
            String line = telnetInputBuffer;
            telnetInputBuffer = "";
            line.trim();
            if (line.length() > 0) {
                extern void handleSerialTestCommand(const String& line);
                handleSerialTestCommand(line);
            }
        } else {
            telnetInputBuffer += c;
        }
    }
}

void TelnetBridge::writeToClient(const char* msg) {
    if (_client && _client.connected()) {
        // Stuur altijd CRLF na elk bericht voor correcte opmaak in Telnet
        _client.write(msg);
        _client.write("\r\n");
    }
}
