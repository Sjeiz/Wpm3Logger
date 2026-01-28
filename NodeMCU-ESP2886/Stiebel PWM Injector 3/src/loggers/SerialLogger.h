#ifndef SERIALLOGGER_H
#define SERIALLOGGER_H
#include "Logger.h"
class SerialLogger : public Logger {
public:
    void log(const String& msg) override;
    void log(const String& msg, LogLevel level) override { log(msg); }
    void logStatus(const StatusInfo& statusInfo) override;
};
#endif // SERIALLOGGER_H
