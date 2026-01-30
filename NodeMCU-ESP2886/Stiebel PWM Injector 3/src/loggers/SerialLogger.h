#ifndef SERIALLOGGER_H
#define SERIALLOGGER_H
#include "Logger.h"
class SerialLogger : public Logger {
public:
    void log(const String& msg, LogLevel level = LogLevel::LOG_NORMAL) override;
    void logStatus(const StatusInfo& statusInfo) override;
};
#endif // SERIALLOGGER_H
