
#include "SerialLogger.h"
#include <Arduino.h>
#include "../globals.h"
#include "../helpers.h"


void SerialLogger::log(const String& msg) {
    String ts = timeStamp();
    String out = "[" + ts + "] " + msg;
    Serial.print(out);
    // Stuur log ook naar Telnet, indien beschikbaar
    if (logManager.getTelnetBridge()) {
        logManager.getTelnetBridge()->writeToClient(out.c_str());
    }
}

void SerialLogger::log(const String& msg, LogLevel level) {
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
    log("[" + ts + "] " + levelStr + msg);
}

void SerialLogger::logStatus(const StatusInfo& statusInfo) {
    this->log(formatStatusLogLine(statusInfo));
}
