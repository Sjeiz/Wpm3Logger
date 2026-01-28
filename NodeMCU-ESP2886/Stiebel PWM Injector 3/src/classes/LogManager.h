 #ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include "../config.h"
#include "../loggers/Logger.h"
#include "../classes/StatusInfo.h"

// Forward declarations to avoid include order issues
class TelnetLogger;
class WebLogger;
#define MAX_LOGGERS 3
class LogManager : public Logger {
public:
    LogManager();
    void addLogger(Logger* logger);
    void log(const String& msg) override;
    void log(const String& msg, LogLevel level);
    void logStatus(const StatusInfo& statusInfo);
    void loop(const StatusInfo& statusInfo);

    TelnetLogger* getTelnetLogger();
    WebLogger* getWebLogger();

private:
    Logger* loggers[MAX_LOGGERS];
    int count;
    unsigned long lastStatusLog = 0;
    const unsigned long statusInterval = 5000; // 5 seconden
    TelnetLogger* telnetLogger;
    WebLogger* webLogger;
};
#endif // LOGMANAGER_H
