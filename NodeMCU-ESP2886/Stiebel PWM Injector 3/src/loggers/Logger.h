#ifndef LOGGER_H
#define LOGGER_H
#include <Arduino.h>
class Logger {
public:
    virtual ~Logger() {}
    virtual void log(const String& msg) = 0;
};
#endif // LOGGER_H
