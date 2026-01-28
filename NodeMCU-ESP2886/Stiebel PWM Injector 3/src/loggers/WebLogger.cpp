
#include "WebLogger.h"
#include "../classes/StatusInfo.h"
#include <Arduino.h>
#include <string.h>

void WebLogger::logStatus(const StatusInfo& statusInfo) {
    String html = "<div class='statuslog'>";
    html += "<b>State:</b> " + statusInfo.stateName + statusInfo.stateTimeStr + "<br>";
    html += "<b>Pomp HK2:</b> " + statusInfo.outputStatus + "<br>";
    html += "<b>Compressor:</b> " + statusInfo.compressorStr + "<br>";
    html += "<b>PWM-out:</b> " + statusInfo.pwmOutVal + "<br>";
    html += "<b>FlowTemp:</b> " + String(statusInfo.flowTemp, 1) + "<br>";
    html += "<b>WiFi:</b> " + String(statusInfo.wifiOk ? "OK" : "FAIL") + "<br>";
    html += "<b>Modbus:</b> " + statusInfo.modbusStr + "<br>";
    html += "</div>\n";
    this->log(html);
}

WebLogger::WebLogger(size_t bufferSize)
    : bufferSize(bufferSize), writePos(0)
{
    buffer = new char[bufferSize];
    buffer[0] = '\0';
}

WebLogger::~WebLogger() {
    delete[] buffer;
}

void WebLogger::begin() {
    clear();
}

void WebLogger::log(const String& message) {
    appendToBuffer(message);
}

void WebLogger::appendToBuffer(const String& message) {
    size_t msgLen = message.length();
    if (msgLen >= bufferSize) {
        strncpy(buffer, message.c_str() + (msgLen - bufferSize + 1), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        writePos = bufferSize - 1;
        return;
    }
    if (writePos + msgLen >= bufferSize) {
        size_t shift = writePos + msgLen - bufferSize + 1;
        memmove(buffer, buffer + shift, bufferSize - shift);
        writePos -= shift;
    }
    memcpy(buffer + writePos, message.c_str(), msgLen);
    writePos += msgLen;
    buffer[writePos] = '\0';
}



// Helper: return log lines in reverse order (newest first)
static String reverseLines(const char* buf) {
    // Collect pointers to the start of each line
    const int maxLines = 512; // Performance limit
    const char* lines[maxLines];
    int lineCount = 0;
    const char* p = buf;
    lines[0] = p;
    while (*p && lineCount < maxLines-1) {
        if (*p == '\n') {
            if (*(p+1)) {
                lines[++lineCount] = p+1;
            }
        }
        ++p;
    }
    // Build string in reverse order
    String out;
    for (int i = lineCount; i >= 0; --i) {
        const char* start = lines[i];
        const char* end = (i == lineCount) ? p : lines[i+1]-1;
        int len = end - start;
        if (len > 0) {
            char tmp[512];
            if (len > (int)sizeof(tmp)-2) len = sizeof(tmp)-2;
            memcpy(tmp, start, len);
            tmp[len] = '\0';
            out += tmp;
            out += "\r\n";
        }
    }
    return out;
}

String WebLogger::getLogHtml() const {
    String html = F("<pre style='font-family:monospace;font-size:12px;'>");
    html += reverseLines(buffer);
    html += F("</pre>");
    return html;
}

String WebLogger::getLogText() const {
    return reverseLines(buffer);
}

void WebLogger::clear() {
    buffer[0] = '\0';
    writePos = 0;
}
