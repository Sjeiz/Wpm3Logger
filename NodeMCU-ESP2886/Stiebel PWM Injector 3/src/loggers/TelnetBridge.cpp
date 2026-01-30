#include "TelnetBridge.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "../helpers.h"

void TelnetBridge::begin() {
    if (_server) _server->begin();
}

// Telnet command handler (geen UART bridge)
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
        // Filter Telnet control codes (IAC = 255 and following negotiation bytes)
        static bool inTelnetCommand = false;
        if ((uint8_t)c == 255) { // IAC
            inTelnetCommand = true;
            continue;
        }
        if (inTelnetCommand) {
            // Skip the next 2 bytes after IAC (WILL/DO/DONT/WONT + option)
            static int skipCount = 2;
            --skipCount;
            if (skipCount == 0) {
                inTelnetCommand = false;
                skipCount = 2;
            }
            continue;
        }
        if (c == '\r') continue; // ignore CR
        if (c == '\n') {
            String line = telnetInputBuffer;
            telnetInputBuffer = "";
            line.trim();
            if (line.length() > 0) {
                logMessage(String("[TELNET] Ontvangen: ") + line);
                extern void handleSerialTestCommand(const String& line);
                handleSerialTestCommand(line);
            }
        } else if (c >= 32 && c <= 126) { // Only printable ASCII
            telnetInputBuffer += c;
        }
        // else: ignore non-printable
    }
}

void TelnetBridge::writeToClient(const char* msg) {
    if (_client && _client.connected()) {
        // Stuur altijd CRLF na elk bericht voor correcte opmaak in Telnet
        _client.write(msg);
        _client.write("\r\n");
    }
}
