
#ifndef WEBLOGGER_H
#define WEBLOGGER_H

#include <Arduino.h>
#include "../loggers/Logger.h"

class StatusInfo;

class WebLogger : public Logger {
public:
    WebLogger(size_t maxLines = WEBLOGGER_BUFFER_LINES);

    void begin();
    void log(const String& message, LogLevel level = LogLevel::LOG_NORMAL) override;
    void logStatus(const StatusInfo& statusInfo) override;
    String getLogHtml() const;
    void clear();

private:
    std::vector<String> logLines;
    size_t maxLines;
};

#endif // WEBLOGGER_H
