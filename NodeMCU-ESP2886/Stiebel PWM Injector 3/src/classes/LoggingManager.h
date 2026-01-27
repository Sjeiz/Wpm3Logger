#ifndef LOGGINGMANAGER_H
#define LOGGINGMANAGER_H
#include "../loggers/Logger.h"
#define MAX_LOGGERS 3
class LoggingManager : public Logger {
public:
    LoggingManager();
    void addLogger(Logger* logger);
    void log(const String& msg) override;
private:
    Logger* loggers[MAX_LOGGERS];
    int count;
};
#endif // LOGGINGMANAGER_H
