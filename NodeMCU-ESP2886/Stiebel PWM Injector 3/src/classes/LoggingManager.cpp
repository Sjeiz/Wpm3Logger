#include "LogManager.h"
LogManager::LogManager() : count(0) {}
void LogManager::addLogger(Logger* logger) {
    if (count < MAX_LOGGERS) loggers[count++] = logger;
}
void LogManager::log(const String& msg) {
    for (int i = 0; i < count; ++i) {
        if (loggers[i]) loggers[i]->log(msg);
    }
}
