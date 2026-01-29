

#include "config.h"
#include "OutputManager.h"
#include "../include/config.h"
#include <Ticker.h>

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
    // Output aansturing alleen bij echte state-wissel
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
    // Outputs naar veilige standaardstate
    digitalWrite(PIN_PUMP_FORCE, LOW); // Pump override off
    digitalWrite(PIN_PUMP_BLOCKED, LOW); // Pomp niet geblokkeerd
    analogWrite(PIN_PWM_OUT, 0);    // PWM uit
    digitalWrite(LED_BUILTIN, HIGH); // LED uit
    ledTicker.detach();
    ledToggle(100, 900, false); // Start met langzame knipper (normaal), LED uit
}
void OutputManager::setDefrost() {
    digitalWrite(PIN_PUMP_FORCE, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, HIGH);
    analogWrite(PIN_PWM_OUT, 0);
    ledTicker.detach();
    ledToggle(100, 100, false); // Snel knipperen, LED uit
    Serial.println("\n--- Setting DEFROST output state: Snel knipperen ---\n");
}
void OutputManager::setPostRun(int pwmPercent) {
    digitalWrite(PIN_PUMP_FORCE, HIGH);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, (int)(pwmPercent * 1023 / 100));
    ledTicker.detach();
    ledToggle(1000, 1000, false); // 1 sec aan/uit (nu 0.5s/0.5s), LED uit
    Serial.println("\n--- Setting POST_RUN output state: 1 sec aan/uit ---\n");
}
void OutputManager::setNormal() {
    digitalWrite(PIN_PUMP_FORCE, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
    analogWrite(PIN_PWM_OUT, 0);
    ledTicker.detach();
    digitalWrite(LED_BUILTIN, HIGH); // Start met LED uit
    ledToggle(100, 800, false); // Start flitspatroon, LED uit
    Serial.println("\n--- Setting NORMAL output state: korte flits ---\n");
}
