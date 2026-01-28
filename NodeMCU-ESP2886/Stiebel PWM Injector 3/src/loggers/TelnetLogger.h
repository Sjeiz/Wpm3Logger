#ifndef TELNETLOGGER_H
#define TELNETLOGGER_H
#include "Logger.h"
#include <ESP8266WiFi.h>
class TelnetLogger : public Logger {
public:
    TelnetLogger(WiFiServer* server);
    void log(const String& msg) override;
    void log(const String& msg, LogLevel level) override { log(msg); }
    void logStatus(const StatusInfo& statusInfo) override;
    void handleClient();
private:
    WiFiServer* _server;
    WiFiClient _client;
};
#endif // TELNETLOGGER_H
