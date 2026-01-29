#ifndef TELNETBRIDGE_H
#define TELNETBRIDGE_H
#include <ESP8266WiFi.h>
class TelnetBridge {
public:
    TelnetBridge(WiFiServer* server);
    void begin();
    void handleClient();
    void writeToClient(const char* msg);
private:
    WiFiServer* _server;
    WiFiClient _client;
    String telnetInputBuffer;
};
#endif // TELNETBRIDGE_H
