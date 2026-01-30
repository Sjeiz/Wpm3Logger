
#include "WebLogger.h"
#include <ESP8266WebServer.h>
#include "../config.h"
#include "../helpers.h"
#include "../classes/StatusInfo.h"
#include <stdio.h>


void WebLogger::streamLogHtml(ESP8266WebServer& server) const {
    server.sendContent("<pre style='font-family:monospace;font-size:12px;'>");
    // Print from newest to oldest (newest first)
    for (size_t i = 0; i < logCount; ++i) {
        size_t idx = (logHead + logCount - 1 - i + WEBLOGGER_LINES_COUNT) % WEBLOGGER_LINES_COUNT;
        String htmlLine = String(logBuffer[idx]);
        htmlLine.replace("&", "&amp;");
        htmlLine.replace("<", "&lt;");
        htmlLine.replace(">", "&gt;");
        htmlLine.replace("\n", "<br>");
        server.sendContent(htmlLine);
    }
    server.sendContent("</pre>");
}


// Only log if relevant statusInfo fields differ from previous (excluding stateTimeStr)
void WebLogger::logStatus(const StatusInfo& statusInfo) {
    static StatusInfo lastStatusInfo; // default init, always different the first time
    static unsigned long lastDetailLogMs = 0;
    bool changed = false;
    changed |= statusInfo.stateName != lastStatusInfo.stateName;
    changed |= statusInfo.outputStatus != lastStatusInfo.outputStatus;
    changed |= statusInfo.compressorStr != lastStatusInfo.compressorStr;
    changed |= statusInfo.pwmOutVal != lastStatusInfo.pwmOutVal;
    // Only log significant temperature changes (margin ±0.2)
    changed |= fabs(statusInfo.flowTemp - lastStatusInfo.flowTemp) > WEBLOGGER_TEMP_DELTA;
    changed |= statusInfo.wifiOk != lastStatusInfo.wifiOk;
    changed |= statusInfo.modbusStr != lastStatusInfo.modbusStr;
    unsigned long nowMs = millis();
    unsigned long intervalMs = WEBLOGGER_DETAIL_INTERVAL_SEC * 1000UL;
    bool timeElapsed = (nowMs - lastDetailLogMs) >= intervalMs;
    if (changed || timeElapsed) {
        this->log(formatStatusLogLine(statusInfo));
        lastStatusInfo = statusInfo;
        lastDetailLogMs = nowMs;
    }
}





WebLogger::WebLogger() {}


void WebLogger::begin() {
    clear();
}




void WebLogger::log(const String& message, LogLevel level) {
    if(level == LogLevel::LOG_DEBUG && !DEBUG) return;
    if(level == LogLevel::LOG_VERBOSE && !(VERBOSE || DEBUG)) return;
    String ts = timeStamp();
    String levelStr;
    switch (level) {
        case LogLevel::LOG_DEBUG: levelStr = "[DEBUG] "; break;
        case LogLevel::LOG_VERBOSE: levelStr = "[VERBOSE] "; break;
        case LogLevel::LOG_NORMAL:
        default: levelStr = ""; break;
    }
    String msg = "[" + ts + "] " + levelStr + message;
    if (!msg.endsWith("\n")) msg += "\n";
    // Truncate if too long
    msg = msg.substring(0, WEBLOGGER_LINES_LENGTH - 1);
    // Insert at head
    strncpy(logBuffer[(logHead + logCount) % WEBLOGGER_LINES_COUNT], msg.c_str(), WEBLOGGER_LINES_LENGTH);
    if (logCount < WEBLOGGER_LINES_COUNT) {
        ++logCount;
    } else {
        logHead = (logHead + 1) % WEBLOGGER_LINES_COUNT;
    }
}

String WebLogger::getLogHtml() const {
    String html = F("<pre style='font-family:monospace;font-size:12px;'>");
    for (size_t i = 0; i < logCount; ++i) {
        size_t idx = (logHead + logCount - 1 - i + WEBLOGGER_LINES_COUNT) % WEBLOGGER_LINES_COUNT;
        String line = String(logBuffer[idx]);
        line.replace("\n", "<br>");
        html += line;
    }
    html += F("</pre>");
    return html;
}



void WebLogger::clear() {
    for (size_t i = 0; i < WEBLOGGER_LINES_COUNT; ++i) {
        logBuffer[i][0] = '\0';
    }
    logHead = 0;
    logCount = 0;
}
