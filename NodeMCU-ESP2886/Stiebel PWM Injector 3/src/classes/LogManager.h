 #ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include "../config.h"
#include "../loggers/Logger.h"
#include "../classes/StatusInfo.h"

// Forward declarations to avoid include order issues
class TelnetBridge;
class WebLogger;
#define MAX_LOGGERS 3
class LogManager : public Logger {
public:
    LogManager();
    void addLogger(Logger* logger);
    void log(const String& msg, LogLevel level = LogLevel::LOG_NORMAL) override;
    void logStatus(const StatusInfo& statusInfo) override;
    void loop(const StatusInfo& statusInfo);

    TelnetBridge* getTelnetBridge();
    WebLogger* getWebLogger();

private:
    Logger* loggers[MAX_LOGGERS];
    int count;
    unsigned long lastStatusLog = 0;
    const unsigned long statusInterval = 5000; // 5 seconden
    TelnetBridge* telnetBridge;
    WebLogger* webLogger;
};
#endif // LOGMANAGER_H
