#include "LogManager.h"
#include "StatusInfo.h"


#include "../loggers/TelnetLogger.h"
#include "../loggers/WebLogger.h"
#include <ESP8266WiFi.h>

LogManager::LogManager()
    : count(0), lastStatusLog(0) {
    telnetLogger = new TelnetLogger(new WiFiServer(23));
    webLogger = new WebLogger(WEBLOGGER_BUFFER_SIZE);
}

TelnetLogger* LogManager::getTelnetLogger() {
    return telnetLogger;
}

WebLogger* LogManager::getWebLogger() {
    return webLogger;
}

void LogManager::addLogger(Logger* logger) {
    if (count < MAX_LOGGERS) {
        loggers[count++] = logger;
    }
}

void LogManager::log(const String& msg) {
    log(msg, LogLevel::LOG_NORMAL);
}

void LogManager::log(const String& msg, LogLevel level) {
    for (int i = 0; i < count; ++i) {
        loggers[i]->log(msg, level);
    }
}

void LogManager::logStatus(const StatusInfo& statusInfo) {
    for (int i = 0; i < count; ++i) {
        loggers[i]->logStatus(statusInfo);
    }
}

void LogManager::loop(const StatusInfo& statusInfo) {
    unsigned long now = millis();
    if (now - lastStatusLog >= statusInterval) {
        lastStatusLog = now;
        logStatus(statusInfo);
    }
}
