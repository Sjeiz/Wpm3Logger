#include "config.h"
#include "OutputManager.h"
#include "../include/config.h"
#include <Ticker.h>
#include "../helpers.h"

// Prototype for local use in this file
void ledToggle(unsigned int onDuration, unsigned int offDuration, bool ledIsAan);

static Ticker ledTicker;

void ledToggle(unsigned int onDuration, unsigned int offDuration, bool ledIsAan) {
    digitalWrite(LED_BUILTIN, ledIsAan ? LOW : HIGH);
    ledTicker.attach_ms(ledIsAan ? onDuration : offDuration, [onDuration, offDuration, ledIsAan]() {
        ledToggle(onDuration, offDuration, !ledIsAan);
    });
}


void OutputManager::loop(const char* stateName) {
    // Only update outputs on actual state change
    if (!stateName) return;
    if (lastStateName && strcmp(stateName, lastStateName) == 0) return;
    if (strcmp(stateName, "DEFROST") == 0) {
        setDefrost();
    } else if (strcmp(stateName, "POST_RUN") == 0) {
        setPostRun(PWM_OUT_DUTY_PERCENT);
    } else {
        setNormal();
    }
    lastStateName = stateName;
}

void OutputManager::begin() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_PUMP_FORCE, OUTPUT);
    pinMode(PIN_PUMP_BLOCKED, OUTPUT);
    pinMode(PIN_PWM_OUT, OUTPUT);
    analogWriteFreq(PWM_OUT_FREQUENCY_HZ);
    // Set outputs directly to safe default state
    setNormal(); 
}
void OutputManager::setDefrost() {
    digitalWrite(PIN_PUMP_FORCE, LOW);
    // DO NOT block the pump, otherwise the heat pump will enable the electric heater (NHZ)!
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, 0);
    currentPwmPercent = 0;
    digitalWrite(LED_BUILTIN, HIGH); // LED off
    ledTicker.detach();
    ledToggle(100, 100, false); // Fast blinking
}
void OutputManager::setPostRun(int pwmPercent) {
    digitalWrite(PIN_PUMP_FORCE, HIGH); // Pump forced ON in post-run
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    // Inverteer PWM voor open collector met pull-up
    int pwmValue = (int)((100 - pwmPercent) * 1023 / 100);
    analogWrite(PIN_PWM_OUT, pwmValue);
    currentPwmPercent = pwmPercent;
    logMessage(String("PWM-out set to ") + String(pwmValue) + " (duty " + String(pwmPercent) + "%, inverted)", LogLevel::LOG_DEBUG);
    ledTicker.detach();
    ledToggle(1000, 1000, false); // 1 second on/off
}
void OutputManager::setNormal() {
    digitalWrite(PIN_PUMP_FORCE, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, 0);
    currentPwmPercent = 0;
    ledTicker.detach();
    digitalWrite(LED_BUILTIN, HIGH); // Start with LED off
    ledToggle(100, 800, false); // Start flash pattern, LED off
}
