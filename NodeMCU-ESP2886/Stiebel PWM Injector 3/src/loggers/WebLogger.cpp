#include <Arduino.h>
#include "WebLogger.h"
#include "../classes/StatusInfo.h"
#include <stdio.h>


// Log alleen als relevante statusInfo-velden verschillen van vorige (excl. stateTimeStr)
void WebLogger::logStatus(const StatusInfo& statusInfo) {
    static StatusInfo lastStatusInfo; // default init, eerste keer altijd anders
    bool changed = false;
    changed |= statusInfo.stateName != lastStatusInfo.stateName;
    changed |= statusInfo.outputStatus != lastStatusInfo.outputStatus;
    changed |= statusInfo.compressorStr != lastStatusInfo.compressorStr;
    changed |= statusInfo.pwmOutVal != lastStatusInfo.pwmOutVal;
    changed |= statusInfo.flowTemp != lastStatusInfo.flowTemp;
    changed |= statusInfo.wifiOk != lastStatusInfo.wifiOk;
    changed |= statusInfo.modbusStr != lastStatusInfo.modbusStr;
    if (changed) {
        char timebuf[20];
        time_t now = time(nullptr);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        char buf[256];
        snprintf(buf, sizeof(buf),
            "[%s] State: %s (%s)  PumpHK2: %s  Compressor: %s  PWM-out: %s  FlowTemp: %.1f  WiFi: %s  Modbus: %s",
            timebuf,
            statusInfo.stateName.c_str(),
            statusInfo.stateTimeStr.c_str(),
            statusInfo.outputStatus.c_str(),
            statusInfo.compressorStr.c_str(),
            statusInfo.pwmOutVal.c_str(),
            statusInfo.flowTemp,
            statusInfo.wifiOk ? "OK" : "FAIL",
            statusInfo.modbusStr.c_str()
        );
        this->log(String(buf) + "\n");
        lastStatusInfo = statusInfo;
    }
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
        // Regel is te groot voor de buffer, sla hem over
        return;
    }

    // Verwijder onderaan oude regels tot er ruimte is
    while (writePos + msgLen >= bufferSize) {
        // Zoek laatste newline (onderste/oudste regel)
        char* lastNewline = strrchr(buffer, '\n');
        if (lastNewline) {
            size_t removeLen = (buffer + writePos) - lastNewline;
            writePos -= removeLen;
            buffer[writePos] = '\0';
        } else {
            // Geen newline, buffer leegmaken
            buffer[0] = '\0';
            writePos = 0;
            break;
        }
    }

    // Schuif bestaande buffer op om ruimte te maken aan het begin
    memmove(buffer + msgLen, buffer, writePos);
    memcpy(buffer, message.c_str(), msgLen);
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
    return String(buffer);
}

void WebLogger::clear() {
    buffer[0] = '\0';
    writePos = 0;
}
