#ifndef TELNETBRIDGE_H
#define TELNETBRIDGE_H
#include <ESP8266WiFi.h>
class TelnetBridge {
public:
    TelnetBridge(WiFiServer* server);
    void handleClient();
private:
    WiFiServer* _server;
    WiFiClient _client;
};
#endif // TELNETBRIDGE_H
