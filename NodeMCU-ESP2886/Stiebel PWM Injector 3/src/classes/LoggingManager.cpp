#include "LoggingManager.h"
LoggingManager::LoggingManager() : count(0) {}
void LoggingManager::addLogger(Logger* logger) {
    if (count < MAX_LOGGERS) loggers[count++] = logger;
}
void LoggingManager::log(const String& msg) {
    for (int i = 0; i < count; ++i) {
        if (loggers[i]) loggers[i]->log(msg);
    }
}
