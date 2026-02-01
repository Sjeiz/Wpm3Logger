
#ifndef WEBLOGGER_H
#define WEBLOGGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <stdio.h>
#define WEBLOGGER_LINES_COUNT 150   // must be a compiler value, so it cannot be declared in config
#define WEBLOGGER_LINES_LENGTH 185  // must be a compiler value, so it cannot be declared in config
#include "../config.h"
#include "../classes/StatusInfo.h"
#include "../loggers/Logger.h"
#include "../helpers.h"

class StatusInfo;

class WebLogger : public Logger {
public:
    WebLogger();
    void begin();
    void log(const String& message, LogLevel level = LogLevel::LOG_NORMAL) override;
    void logStatus(const StatusInfo& statusInfo) override;
    String getLogHtml() const;
    void streamLogHtml(ESP8266WebServer& server) const;
    void clear();

private:
    char logBuffer[WEBLOGGER_LINES_COUNT][WEBLOGGER_LINES_LENGTH];
    size_t logHead = 0;
    size_t logCount = 0;
};

#endif // WEBLOGGER_H
