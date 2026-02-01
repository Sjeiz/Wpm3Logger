
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
    // --- Detailregel criteria ---
    // 1. Minimaal 1 update per WEBLOGGER_DETAIL_INTERVAL_SEC
    // 2. Update bij statechange
    // 3. Update bij pump mode change
    // 4. Update bij compressor change
    // 5. Update bij PWM-in change > WEBLOGGER_PWMIN_DELTA_PCT
    // 6. Update bij flowtemp change > WEBLOGGER_TEMP_DELTA
    // 7. Update bij flowrate change > WEBLOGGER_FLOW_DELTA_PCT
    // 8. Update bij Wifi change
    // 9. Update bij modbus change

    static StatusInfo prev = {};
    static unsigned long lastDetail = 0;
    unsigned long now = millis();
    bool logDetail = false;

    // 1. Minimaal 1 update per interval
    if (now - lastDetail > WEBLOGGER_DETAIL_INTERVAL_SEC * 1000UL) logDetail = true;
    // 2. Statechange
    if (strcmp(statusInfo.stateName, prev.stateName) != 0) logDetail = true;
    // 3. Pump mode change
    if (strcmp(statusInfo.outputStatus, prev.outputStatus) != 0) logDetail = true;
    // 4. Compressor change
    if (strcmp(statusInfo.compressorStr, prev.compressorStr) != 0) logDetail = true;
    // 5. PWM-in change
    float pwmInNow = atof(statusInfo.pwmInVal);
    float pwmInPrev = atof(prev.pwmInVal);
    if (fabs(pwmInNow - pwmInPrev) >= WEBLOGGER_PWMIN_DELTA_PCT) logDetail = true;
    // 6. Flowtemp change
    if (fabs(statusInfo.flowTemp - prev.flowTemp) >= WEBLOGGER_TEMP_DELTA) logDetail = true;
    // 7. Flowrate change
    float flowPctNow = statusInfo.flowRate;
    float flowPctPrev = prev.flowRate;
    if (flowPctPrev > 0 && fabs(flowPctNow - flowPctPrev) / flowPctPrev * 100.0f >= WEBLOGGER_FLOW_DELTA_PCT) logDetail = true;
    // 8. Wifi change
    if (statusInfo.wifiOk != prev.wifiOk) logDetail = true;
    // 9. Modbus change
    if (strcmp(statusInfo.modbusStr, prev.modbusStr) != 0) logDetail = true;

    if (logDetail) {
        this->log(formatStatusLogLine(statusInfo));
        lastDetail = now;
        prev = statusInfo;
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
    // Truncate if too long, but always end with ...\n if truncated
    if (msg.length() >= WEBLOGGER_LINES_LENGTH) {
        // Reserve 4 chars for ...\n
        msg = msg.substring(0, WEBLOGGER_LINES_LENGTH - 4) + "...\n";
    }
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
