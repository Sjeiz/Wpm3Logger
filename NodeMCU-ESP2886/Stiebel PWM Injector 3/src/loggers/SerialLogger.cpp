#include "SerialLogger.h"
#include <Arduino.h>
void SerialLogger::log(const String& msg) {
    Serial.print(msg);
}
