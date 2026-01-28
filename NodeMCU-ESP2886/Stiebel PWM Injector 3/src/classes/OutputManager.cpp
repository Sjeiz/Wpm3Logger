#include "config.h"
#include "OutputManager.h"
#include "../include/config.h"
void OutputManager::begin() {
    pinMode(PIN_PUMP_ON, OUTPUT);
    pinMode(PIN_PUMP_BLOCKED, OUTPUT);
    pinMode(PIN_PWM_OUT, OUTPUT);
    analogWriteFreq(PWM_OUT_FREQUENCY_HZ);
    int pwmValue = (int)(PWM_OUT_DUTY_PERCENT * 1023 / 100);
    analogWrite(PIN_PWM_OUT, pwmValue);
}
void OutputManager::setDefrost() {
    digitalWrite(PIN_PUMP_ON, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, HIGH);
    analogWrite(PIN_PWM_OUT, 0);
}
void OutputManager::setPostRun(int pwmPercent) {
    digitalWrite(PIN_PUMP_ON, HIGH);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, (int)(pwmPercent * 1023 / 100));
}
void OutputManager::setNormal() {
    digitalWrite(PIN_PUMP_ON, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, 0);
}
