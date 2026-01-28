#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include "../loggers/Logger.h"
#define MAX_LOGGERS 3
class LogManager : public Logger {
public:
    LogManager();
    void addLogger(Logger* logger);
    void log(const String& msg) override;
private:
    Logger* loggers[MAX_LOGGERS];
    int count;
};
#endif // LOGMANAGER_H
