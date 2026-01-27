#ifndef SERIALLOGGER_H
#define SERIALLOGGER_H
#include "Logger.h"
class SerialLogger : public Logger {
public:
    void log(const String& msg) override;
};
#endif // SERIALLOGGER_H
