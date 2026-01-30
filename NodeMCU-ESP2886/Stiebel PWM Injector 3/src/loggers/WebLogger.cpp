
#include <Arduino.h>
#include "WebLogger.h"
#include "../classes/StatusInfo.h"
#include <stdio.h>
#include <vector>
#include "../helpers.h"


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
    changed |= fabs(statusInfo.flowTemp - lastStatusInfo.flowTemp) > WEBLOGGER_TEMP_MARGE;
    changed |= statusInfo.wifiOk != lastStatusInfo.wifiOk;
    changed |= statusInfo.modbusStr != lastStatusInfo.modbusStr;
    unsigned long nowMs = millis();
    unsigned long intervalMs = 1000; // * WEBLOGGER_DETAIL_INTERVAL_MIN * 60UL * 1000UL;
    bool timeElapsed = (nowMs - lastDetailLogMs) >= intervalMs;
    if (changed || timeElapsed) {
        this->log(formatStatusLogLine(statusInfo));
        lastStatusInfo = statusInfo;
        lastDetailLogMs = nowMs;
    }
}




WebLogger::WebLogger(size_t maxLines)
    : maxLines(maxLines)
{
}


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
    logLines.insert(logLines.begin(), msg);
    while (logLines.size() > maxLines) {
        logLines.pop_back();
    }
}

String WebLogger::getLogHtml() const {
    String html = F("<pre style='font-family:monospace;font-size:12px;'>");
    String text;
    for (const auto& line : logLines) {
        text += line;
    }
    text.replace("\n", "<br>");
    html += text;
    html += F("</pre>");
    return html;
}



void WebLogger::clear() {
    logLines.clear();
}
