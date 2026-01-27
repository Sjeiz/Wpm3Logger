#include "TelnetLogger.h"
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
