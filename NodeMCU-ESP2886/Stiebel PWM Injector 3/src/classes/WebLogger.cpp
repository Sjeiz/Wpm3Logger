#include "WebLogger.h"
#include <Arduino.h>
#include <string.h>

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
