
#include <Arduino.h>
#include "WebLogger.h"
#include "../classes/StatusInfo.h"
#include <stdio.h>
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
    unsigned long intervalMs = WEBLOGGER_DETAIL_INTERVAL_MIN * 60UL * 1000UL;
    bool timeElapsed = (nowMs - lastDetailLogMs) >= intervalMs;
    if (changed || timeElapsed) {
        this->log(formatStatusLogLine(statusInfo));
        lastStatusInfo = statusInfo;
        lastDetailLogMs = nowMs;
    }
}

WebLogger::WebLogger(size_t bufferSize)
    : bufferSize(2048), writePos(0)
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
    String ts = timeStamp();
    appendToBuffer("[" + ts + "] " + message);
}

void WebLogger::log(const String& message, LogLevel level) {

    if(level == LogLevel::LOG_DEBUG && !DEBUG) return; // Alleen debug tonen als DEBUG==1
    if(level == LogLevel::LOG_VERBOSE && !(VERBOSE || DEBUG)) return; // Alleen verbose tonen als VERBOSE==1 of DEBUG==1

    String ts = timeStamp();
    String levelStr;
    switch (level) {
        case LogLevel::LOG_DEBUG: levelStr = "[DEBUG] "; break;
        case LogLevel::LOG_VERBOSE: levelStr = "[VERBOSE] "; break;
        case LogLevel::LOG_NORMAL:
        default: levelStr = ""; break;
    }
    appendToBuffer("[" + ts + "] " + levelStr + message);
}

void WebLogger::appendToBuffer(const String& message) {
    String msg = message;
    if (!msg.endsWith("\n")) msg += "\n";
    size_t msgLen = msg.length();
    if (msgLen >= bufferSize) {
        // Line is too large for the buffer, skip it
        return;
    }

    // Log en wacht 10s voordat we regels gaan verwijderen als de buffer vol dreigt te raken
    if (writePos + msgLen >= bufferSize) {
        logMessage("⚠️ WebLogger buffer bijna vol, ga oude regels verwijderen na 10s delay...");
        delay(10000);
    }
    // Remove oldest lines at the end (onderaan) totdat er ruimte is
    while (writePos + msgLen >= bufferSize) {
        // Zoek de laatste '\n' (onderaan, dus vanaf het einde zoeken)
        char* lastNl = nullptr;
        for (char* p = buffer + writePos - 2; p >= buffer; --p) {
            if (*p == '\n') {
                lastNl = p;
                break;
            }
        }
        if (lastNl) {
            size_t removeLen = (buffer + writePos) - (lastNl + 1); // +1 om '\n' zelf ook te verwijderen
            memmove(buffer, lastNl + 1, removeLen);
            writePos -= (lastNl + 1 - buffer);
            buffer[writePos] = '\0';
        } else {
            // Geen '\n' meer, buffer leegmaken
            buffer[0] = '\0';
            writePos = 0;
            break;
        }
    }

    // Shift bestaande buffer naar rechts om ruimte bovenaan te maken
    memmove(buffer + msgLen, buffer, writePos);
    memcpy(buffer, msg.c_str(), msgLen);
    writePos += msgLen;
    buffer[writePos] = '\0';
}



String WebLogger::getLogHtml() const {
    String html = F("<pre style='font-family:monospace;font-size:12px;'>");
    html += buffer;
    html += F("</pre>");
    return html;
}

String WebLogger::getLogText() const {
    String text = String(buffer);
    text.replace("<br>", "\n");
    return text;
}

void WebLogger::clear() {
    buffer[0] = '\0';
    writePos = 0;
}
