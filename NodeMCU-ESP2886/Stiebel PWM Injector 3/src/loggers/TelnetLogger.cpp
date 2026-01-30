
#include "TelnetLogger.h"
#include <Arduino.h>
#include "../classes/StatusInfo.h"
#include <stdio.h>
#include <time.h>
#include "../helpers.h"



void TelnetLogger::log(const String& msg, LogLevel level) {
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
    String out = "[" + ts + "] " + levelStr + msg;
    if (_bridge) {
        _bridge->writeToClient(out.c_str());
    }
}

void TelnetLogger::logStatus(const StatusInfo& statusInfo) {
    this->log(formatStatusLogLine(statusInfo));
}

