 #ifndef LOGGER_H
#define LOGGER_H
#include <Arduino.h>
#include "../config.h"
#include "../classes/StatusInfo.h"

class Logger {
public:
    virtual ~Logger() {}
    virtual void log(const String& msg, LogLevel level = LogLevel::LOG_NORMAL) = 0;
    virtual void logStatus(const StatusInfo& info) = 0;
};
#endif // LOGGER_H
