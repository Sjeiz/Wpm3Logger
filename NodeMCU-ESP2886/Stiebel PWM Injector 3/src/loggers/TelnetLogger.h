#ifndef TELNETLOGGER_H
#define TELNETLOGGER_H
#include "../loggers/Logger.h"
#include "TelnetBridge.h"

class TelnetLogger : public Logger {
public:
    TelnetLogger(TelnetBridge* bridge) : _bridge(bridge) {}
    void log(const String& msg) override;
    void log(const String& msg, LogLevel level) override;
    void logStatus(const StatusInfo& statusInfo) override;
private:
    TelnetBridge* _bridge;
};

#endif // TELNETLOGGER_H

