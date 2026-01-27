#ifndef WEBLOGGER_H
#define WEBLOGGER_H

#include <Arduino.h>

class WebLogger {
public:
    WebLogger(size_t bufferSize = 2048);
    ~WebLogger();

    void begin();
    void log(const String& message);
    String getLogHtml() const;
    String getLogText() const;
    void clear();

private:
    char* buffer;
    size_t bufferSize;
    size_t writePos;
    void appendToBuffer(const String& message);
};

#endif // WEBLOGGER_H
