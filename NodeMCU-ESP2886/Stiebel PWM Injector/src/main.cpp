// pwm D6 input tester

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include "Config.h"
#include "PWMDetector.h"
#include "WiFiManager.h"
#include "HeatPumpStateMachine.h"
#include "TemperatureSensor.h"

// ===== FUNCTIONALITY OVERVIEW =====
// This application monitors a PWM signal from a heat pump and intelligently manages
// a floor heating pump based on the heat pump's operational state. It implements:
//
// 1. PWM INPUT DETECTION (D6):
//    - Interrupt-based edge detection with debouncing (100-150 Hz expected)
//    - Open collector optocoupler inverts logic: LOW=active/100%, HIGH=inactive/0%
//    - DC level detection: 100% duty = constant LOW, 0% duty = constant HIGH (no signal)
//    - Config: PWM_DEBOUNCE_TIME, PWM_DETECTION_TIMEOUT
//
// 2. HEAT PUMP STATE MACHINE (6 States):
//    - STANDBY: PWM-in OFF, no post-run timer active
//    - STARTUP: PWM-in ON, waiting 2 minutes for mode detection
//    - HOT_WATER: Temp >40°C (domestic hot water production)
//    - HEATING: Stable/rising temp during startup (space heating mode)
//    - COOLING: Temp drops >1°C during startup (active cooling mode)
//    - POST_RUN: PWM-in OFF after HOT_WATER, 30-minute timer for floor cooling
//
// 3. TEMPERATURE-BASED MODE DETECTION (DS18B20 on D2):
//    - Non-blocking temperature reading with 30-second retry on failure
//    - During 2-minute STARTUP: temp trend determines COOLING (>1°C drop) vs HEATING (stable)
//    - Runtime: only switch to/from HOT_WATER when crossing 40°C threshold
//    - HEATING and COOLING modes remain fixed once determined
//    - Fallback: If sensor unavailable during startup, defaults to HEATING (conservative choice)
//    - Config: TEMP_THRESHOLD_HOT_WATER (40°C), TEMP_TREND_COOLING_THRESHOLD (-1°C)
//
// 4. DEFROST CYCLE DETECTION:
//    - Detected when duty cycle ≥95% AND temperature is falling in HOT_WATER or HEATING modes
//    - Uses temperature trend analysis: falling temp (< -0.3°C over 30s) = defrost, rising = normal
//    - NOT applicable in COOLING mode (100% duty is normal cooling operation)
//    - Triggers PUMP_BLOCKED state to prevent circulation during defrost
//    - Config: DEFROST_DUTY_THRESHOLD (95%)
//
// 5. INTELLIGENT PUMP CONTROL (3 Modes via D7 + D8 relays):
//    - PUMP_FORCED: D7=HIGH (post-run timer) - Forces pump ON for floor cooling
//    - PUMP_BLOCKED: D8=HIGH (defrost cycle) - Blocks pump during defrost
//    - PUMP_NORMAL: Both LOW (normal operation) - Pump follows heat pump control
//    - Priority: POST_RUN > DEFROST > NORMAL
//
// 6. POST-RUN TIMER LOGIC:
//    - Triggered ONLY when transitioning from HOT_WATER state (floor is hottest)
//    - Duration: 30 minutes (configurable)
//    - NOT triggered from HEATING or COOLING states
//    - Cancelled if PWM-in returns (heat pump restarts)
//    - Outputs PWM signal to D5 during post-run (PWM_OUTPUT_FREQ_POSTRUN, PWM_OUTPUT_DUTY_POSTRUN)
//
// 7. LED INDICATOR (Built-in LED):
//    - STANDBY: Short flashes (200ms)
//    - PWM-IN ACTIVE: Slow blink (500ms)
//    - POST-RUN TIMER: Fast blink (100ms)
//    - Config: LED_BLINK_STANDBY, LED_BLINK_SLOW, LED_BLINK_FAST
//
// 8. WIFI & WEB SERVER:
//    - HTTP server on port 80 displays real-time log with auto-refresh
//    - NTP time synchronization (1-min retry if invalid, 1-hour if valid)
//    - Config: WIFI_SSID, WIFI_PW, WEBSERVER_REFRESH_INTERVAL
//
// 9. LOGGING FORMAT (every 30 seconds):
//    - Regular log: [YYYY-MM-DD HH:MM:SS] State: <STATE>  PumpHK2: <STATUS>  PWM-in: <X%|OFF>  PWM-out: <X%|OFF>  Flow: <XX.X|-->°C
//    - Event log: [YYYY-MM-DD HH:MM:SS] === <EVENT MESSAGE> ===
//
// ===== STATE TRANSITION EXAMPLES - COMPLETE OVERVIEW =====
//
// 1. NORMALE STARTUP NAAR HEATING (STABIELE TEMP):
//   [2026-01-24 10:00:00] === HEATPUMP STARTED: Determining mode for 2 minutes... ===
//   [2026-01-24 10:00:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 45.0%  PWM-out: OFF  Flow: 28.5°C
//   [2026-01-24 10:01:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 48.0%  PWM-out: OFF  Flow: 29.0°C
//   [2026-01-24 10:02:00] === MODE DETECTED: Heating Mode ===
//   [2026-01-24 10:02:00] State: HEATING  PumpHK2: NORMAL  PWM-in: 50.0%  PWM-out: OFF  Flow: 29.5°C
//
// 2. STARTUP NAAR COOLING (TEMP DAALT >1°C):
//   [2026-01-24 10:15:00] === HEATPUMP STARTED: Determining mode for 2 minutes... ===
//   [2026-01-24 10:15:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 95.0%  PWM-out: OFF  Flow: 22.5°C
//   [2026-01-24 10:16:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 98.0%  PWM-out: OFF  Flow: 21.0°C
//   [2026-01-24 10:17:00] === MODE DETECTED: Cooling Mode (temp trend) ===
//   [2026-01-24 10:17:00] State: COOLING  PumpHK2: NORMAL  PWM-in: 100.0%  PWM-out: OFF  Flow: 20.0°C
//
// 3. STARTUP NAAR HOT_WATER (TEMP METEEN >40°C):
//   [2026-01-24 11:00:00] === HEATPUMP STARTED: Determining mode for 2 minutes... ===
//   [2026-01-24 11:00:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 60.0%  PWM-out: OFF  Flow: 42.0°C
//   [2026-01-24 11:02:00] === MODE DETECTED: Hot Water Mode ===
//   [2026-01-24 11:02:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 75.0%  PWM-out: OFF  Flow: 48.0°C
//
// 4. HOT WATER NAAR POST-RUN (NORMALE FLOW):
//   [2026-01-24 12:00:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 85.0%  PWM-out: OFF  Flow: 52.3°C
//   [2026-01-24 12:30:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 82.5%  PWM-out: OFF  Flow: 51.8°C
//   [2026-01-24 12:35:15] === HEATPUMP STOPPED: Post-Run Timer started (30.0 min) ===
//   [2026-01-24 12:35:15] State: POST_RUN (30.0 min)  PumpHK2: FORCED  PWM-in: OFF  PWM-out: 30%  Flow: 48.2°C
//   [2026-01-24 12:40:15] State: POST_RUN (25.0 min)  PumpHK2: FORCED  PWM-in: OFF  PWM-out: 30%  Flow: 45.1°C
//   [2026-01-24 13:00:15] State: POST_RUN (5.0 min)  PumpHK2: FORCED  PWM-in: OFF  PWM-out: 30%  Flow: 32.5°C
//   [2026-01-24 13:05:15] === POST-RUN TIMER FINISHED: Entering Standby ===
//   [2026-01-24 13:05:15] State: STANDBY  PumpHK2: NORMAL  PWM-in: OFF  PWM-out: OFF  Flow: 28.5°C
//
// 5. HEATING NAAR STANDBY (GEEN POST-RUN):
//   [2026-01-24 14:00:00] State: HEATING  PumpHK2: NORMAL  PWM-in: 65.0%  PWM-out: OFF  Flow: 35.2°C
//   [2026-01-24 14:15:30] === HEATPUMP STOPPED: Entering Standby ===
//   [2026-01-24 14:15:30] State: STANDBY  PumpHK2: NORMAL  PWM-in: OFF  PWM-out: OFF  Flow: 30.8°C
//
// 6. COOLING NAAR STANDBY (GEEN POST-RUN):
//   [2026-01-24 15:00:00] State: COOLING  PumpHK2: NORMAL  PWM-in: 100.0%  PWM-out: OFF  Flow: 18.5°C
//   [2026-01-24 15:25:45] === HEATPUMP STOPPED: Entering Standby ===
//   [2026-01-24 15:25:45] State: STANDBY  PumpHK2: NORMAL  PWM-in: OFF  PWM-out: OFF  Flow: 22.1°C
//
// 7. POST-RUN TIMER GEANNULEERD DOOR HERSTART:
//   [2026-01-24 16:00:00] State: POST_RUN (15.5 min)  PumpHK2: FORCED  PWM-in: OFF  PWM-out: 30%  Flow: 32.0°C
//   [2026-01-24 16:05:00] === HEATPUMP RESTARTED: Post-Run Timer cancelled, determining mode for 2 minutes... ===
//   [2026-01-24 16:05:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 45.0%  PWM-out: OFF  Flow: 30.5°C
//   [2026-01-24 16:07:00] === MODE DETECTED: Hot Water Mode ===
//   [2026-01-24 16:07:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 78.0%  PWM-out: OFF  Flow: 48.0°C
//
// 8. DEFROST CYCLUS IN HOT WATER MODE:
//   [2026-01-24 17:00:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 80.0%  PWM-out: OFF  Flow: 50.5°C
//   [2026-01-24 17:05:30] === DEFROST CYCLE STARTED ===
//   [2026-01-24 17:05:30] State: HOT_WATER (defrosting)  PumpHK2: BLOCKED  PWM-in: 97.5%  PWM-out: OFF  Flow: 52.8°C
//   [2026-01-24 17:10:00] State: HOT_WATER (defrosting)  PumpHK2: BLOCKED  PWM-in: 98.2%  PWM-out: OFF  Flow: 54.1°C
//   [2026-01-24 17:15:45] === DEFROST CYCLE FINISHED ===
//   [2026-01-24 17:15:45] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 82.0%  PWM-out: OFF  Flow: 51.2°C
//
// 9. DEFROST CYCLUS IN HEATING MODE:
//   [2026-01-24 18:00:00] State: HEATING  PumpHK2: NORMAL  PWM-in: 65.0%  PWM-out: OFF  Flow: 35.2°C
//   [2026-01-24 18:05:30] === DEFROST CYCLE STARTED ===
//   [2026-01-24 18:05:30] State: HEATING (defrosting)  PumpHK2: BLOCKED  PWM-in: 96.8%  PWM-out: OFF  Flow: 38.1°C
//   [2026-01-24 18:15:45] === DEFROST CYCLE FINISHED ===
//   [2026-01-24 18:15:45] State: HEATING  PumpHK2: NORMAL  PWM-in: 68.0%  PWM-out: OFF  Flow: 36.8°C
//
// 10. MODE CHANGE HEATING → HOT_WATER (TEMP >40°C):
//   [2026-01-24 19:00:00] State: HEATING  PumpHK2: NORMAL  PWM-in: 60.0%  PWM-out: OFF  Flow: 38.5°C
//   [2026-01-24 19:10:00] === MODE CHANGE: Heating → Hot Water ===
//   [2026-01-24 19:10:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 75.0%  PWM-out: OFF  Flow: 42.0°C
//
// 11. MODE CHANGE COOLING → HOT_WATER (TEMP >40°C):
//   [2026-01-24 20:00:00] State: COOLING  PumpHK2: NORMAL  PWM-in: 100.0%  PWM-out: OFF  Flow: 35.0°C
//   [2026-01-24 20:15:00] === MODE CHANGE: Cooling → Hot Water ===
//   [2026-01-24 20:15:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 80.0%  PWM-out: OFF  Flow: 43.0°C
//
// 12. MODE CHANGE HOT_WATER → HEATING (TEMP <40°C):
//   [2026-01-24 21:00:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 70.0%  PWM-out: OFF  Flow: 41.5°C
//   [2026-01-24 21:10:00] === MODE CHANGE: Hot Water → Heating ===
//   [2026-01-24 21:10:00] State: HEATING  PumpHK2: NORMAL  PWM-in: 55.0%  PWM-out: OFF  Flow: 38.0°C
//
// 12. MODE CHANGE HOT_WATER → HEATING (TEMP <40°C):
//   [2026-01-24 21:00:00] State: HOT_WATER  PumpHK2: NORMAL  PWM-in: 70.0%  PWM-out: OFF  Flow: 41.5°C
//   [2026-01-24 21:10:00] === MODE CHANGE: Hot Water → Heating ===
//   [2026-01-24 21:10:00] State: HEATING  PumpHK2: NORMAL  PWM-in: 55.0%  PWM-out: OFF  Flow: 38.0°C
//
// 13. STARTUP TIJDENS STARTUP (PWM UIT TIJDENS STARTUP):
//   [2026-01-24 22:00:00] === HEATPUMP STARTED: Determining mode for 2 minutes... ===
//   [2026-01-24 22:00:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 50.0%  PWM-out: OFF  Flow: 28.5°C
//   [2026-01-24 22:01:30] === HEATPUMP STOPPED: Entering Standby ===
//   [2026-01-24 22:01:30] State: STANDBY  PumpHK2: NORMAL  PWM-in: OFF  PWM-out: OFF  Flow: 28.0°C
//
// 14. STARTUP MET TEMPERATUUR SENSOR UITVAL:
//   [2026-01-24 23:00:00] === HEATPUMP STARTED: Determining mode for 2 minutes... ===
//   [2026-01-24 23:00:00] State: STARTUP  PumpHK2: NORMAL  PWM-in: 50.0%  PWM-out: OFF  Flow: --°C
//   [2026-01-24 23:02:00] === MODE DETECTED: Heating Mode (temp sensor unavailable) ===
//   [2026-01-24 23:02:00] State: HEATING  PumpHK2: NORMAL  PWM-in: 60.0%  PWM-out: OFF  Flow: --°C
//
// ALLE MOGELIJKE STATE TRANSITIES:
// • STANDBY → STARTUP (PWM-in ON)
// • STARTUP → HEATING (2 min, temp stable/rising)
// • STARTUP → COOLING (2 min, temp drops >1°C)
// • STARTUP → HOT_WATER (2 min, temp >40°C)
// • STARTUP → STANDBY (PWM-in OFF tijdens startup)
// • HOT_WATER → POST_RUN (PWM-in OFF)
// • HOT_WATER → HEATING (temp daalt onder 40°C)
// • HEATING → HOT_WATER (temp stijgt boven 40°C)
// • HEATING → STANDBY (PWM-in OFF)
// • COOLING → HOT_WATER (temp stijgt boven 40°C)
// • COOLING → STANDBY (PWM-in OFF)
// • POST_RUN → STANDBY (timer afgelopen)
// • POST_RUN → STARTUP (PWM-in ON tijdens post-run)
//
// DEFROST TRANSITIES (binnen states):
// • HOT_WATER → HOT_WATER (defrosting) [duty ≥95%]
// • HEATING → HEATING (defrosting) [duty ≥95%]
// • HOT_WATER (defrosting) → HOT_WATER [duty <95%]
// • HEATING (defrosting) → HEATING [duty <95%]
//
// ===== PWM-IN 100% DETECTIE PROBLEEM & OPLOSSING =====
// PROBLEEM:
// Tijdens defrost stuurt de warmtepomp 100% PWM (constant LOW signaal via open collector).
// Bij afwezigheid van edges werd dit NIET gedetecteerd, en na 30 seconden triggerde de timeout,
// waardoor het systeem dacht dat de warmtepomp gestopt was terwijl deze nog steeds draaide.
//
// Log voorbeeld van het probleem:
// [2026-01-25 01:15:28] State: HEATING  PWM-in: 50.2%  ← laatste edge-based meting
// [2026-01-25 01:15:54] === HEATPUMP STOPPED: Entering Standby ===  ← FOUT! WP draait nog
// [2026-01-25 01:15:54] State: STANDBY  PWM-in: OFF  ← timeout na ~26 seconden
//
// DIAGNOSE:
// 1. PWM gaat van 50% (edges) naar 100% (constant LOW, geen edges meer)
// 2. NO_EDGE_THRESHOLD: Code wacht 1 seconde zonder edges voordat DC detectie start
// 3. DC_DETECTION_THRESHOLD: Code wacht 500ms stabiel level voordat 100% gedetecteerd wordt
// 4. Totaal 1.5 seconden delay - MAAR lastValidMeasurement werd NIET bijgewerkt tijdens deze periode
// 5. Na ~26 sec totaal zonder lastValidMeasurement update → PWM_DETECTION_TIMEOUT (30s) triggert
// 6. State machine denkt dat warmtepomp uit is en gaat naar STANDBY
//
// EERDERE POGINGEN DIE FAALDEN:
// Commit 2180832: DC level detectie toegevoegd (constant LOW = 100%, constant HIGH = 0%)
//                 → Werkte niet: timeout triggerde nog steeds tijdens 1.5 sec wachttijd
// Commit 885f382: "PWM detectie fixes tijdens defrost"
//                 → Werkte niet: lastValidMeasurement werd niet bijgewerkt tijdens overgang
// Commit d525b66: Diverse verbeteringen aan debug output
//                 → Hielp met diagnostiek maar loste timeout probleem niet op
//
// OPLOSSING (25-01-2026):
// In PWMDetector.cpp, updatePWMDetection():
// 1. Tijdens NO_EDGE_THRESHOLD wachttijd: Update lastValidMeasurement als pwmDetected nog waar is
//    → Voorkomt timeout tijdens wachten op begin van DC detectie
// 2. Bij stabiel LOW level >100ms: Update lastValidMeasurement ONMIDDELLIJK
//    → Voorkomt timeout tijdens DC_DETECTION_THRESHOLD wachttijd
// 3. Bij gedetecteerde 100% duty: Continue lastValidMeasurement updates
//    → Voorkomt timeout tijdens langdurige defrost (kan >30 sec duren)
//
// RESULTAAT:
// Er is nu GEEN ENKELE periode waarin lastValidMeasurement niet wordt bijgewerkt tijdens
// de overgang PWM → 100% DC, waardoor timeout NOOIT meer onterecht triggert tijdens defrost.
//
// ===== CONFIGURATION =====
// All configurable parameters are centralized in include/Config.h
// Edit Config.h to modify behavior (pin mappings, timings, WiFi credentials, thresholds, etc.)

ESP8266WebServer server(80);

// State machine and temperature sensor
HeatPumpStateMachine stateMachine;
TemperatureSensor tempSensor(PIN_TEMP_SENSOR);

unsigned long lastBlink = 0;
bool ledState = false;

// Logging and state tracking
unsigned long lastLog = 0;
bool firstLogSent = false;
String lastStatusChange = "";
unsigned long lastNTPSyncAttempt = 0;  // Track NTP resync attempts  // Store last status change message for web display

// Heap warning tracking
bool lowHeapWarned = false;

// Web log buffer (circular buffer for web page display)
String webLogBuffer[WEB_LOG_BUFFER_SIZE];
int webLogIndex = 0;
int webLogCount = 0;

// Forward declarations
String getTimestamp();
String buildStatusLog();  // Build status log message for Serial and Web
void addWebLogLine(String line);  // Add line to web log buffer
void checkHeapAndWarn();          // Warn when heap is low
void logDataLine();               // Log current data line
void onStateMachineEvent(const char* event);  // State machine event callback
void updatePumpControl();         // Update pump relay outputs

// -----------------------------
// Webpagina
// -----------------------------
void handleRoot() {
  // Build HTML header with single snprintf (eliminates String concatenations)
  char htmlHeader[512];
  snprintf(htmlHeader, sizeof(htmlHeader),
    "<html><head><meta http-equiv='refresh' content='%d' />"
    "<style>body{font-family:monospace;margin:20px;font-size:12px;} "
    "pre{background:#f0f0f0;padding:10px;max-height:600px;overflow-y:auto;} "
    ".event{color:blue;margin:10px 0;}</style></head><body>"
    "<h2>PWM Monitor</h2><pre>",
    WEBSERVER_REFRESH_INTERVAL);
  
  String html = String(htmlHeader);
  
  // Show log buffer
  if (webLogCount == 0) {
    html += "No log entries yet...";
  } else {
    // Show all log entries in reverse order (newest first)
    int startIdx = (webLogCount < WEB_LOG_BUFFER_SIZE) ? 0 : webLogIndex;
    for (int i = webLogCount - 1; i >= 0; i--) {
      int idx = (startIdx + i) % WEB_LOG_BUFFER_SIZE;
      html += webLogBuffer[idx] + "\n";
    }
  }
  html += "</pre></body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}

// WiFi connectie
// Opgemerkt: WiFi management is nu in WiFiManager.cpp

// Function to set output PWM parameters
void setOutputPWM(int frequency, int duty) {
  static int lastFrequency = -1;
  static int lastDuty = -1;
  
  // Only log on change
  if (DEBUG_MODE && (frequency != lastFrequency || duty != lastDuty)) {
    Serial.print("DEBUG: setOutputPWM(");
    Serial.print(frequency);
    Serial.print("Hz, ");
    Serial.print(duty);
    Serial.println("%)");
    lastFrequency = frequency;
    lastDuty = duty;
  }
  
  if (duty == 0) {
    // Stop PWM completely
    analogWrite(PIN_PWM_OUT, 0);
    digitalWrite(PIN_PWM_OUT, LOW);
  } else {
    // Set PWM frequency and duty cycle
    analogWriteFreq(frequency);
    int pwmValue = (duty * 1023) / 100;
    analogWrite(PIN_PWM_OUT, pwmValue);
  }
}

// Function to get current time as formatted timestamp string
String getTimestamp() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

// Function to add a line to the web log buffer (circular)
void addWebLogLine(String line) {
  checkHeapAndWarn();

  // Buffer overflow protection: truncate overly long lines
  if (line.length() > 256) {
    line = line.substring(0, 253) + "...";
  }

  webLogBuffer[webLogIndex] = line;
  webLogIndex = (webLogIndex + 1) % WEB_LOG_BUFFER_SIZE;
  if (webLogCount < WEB_LOG_BUFFER_SIZE) {
    webLogCount++;
  }
}

// State machine event callback
void onStateMachineEvent(const char* event) {
  String eventMsg = "[" + getTimestamp() + "] === " + String(event) + " ===";
  Serial.println(eventMsg);
  addWebLogLine(eventMsg);
  lastStatusChange = eventMsg;
  logDataLine();  // Log complete status line after event
}

// Update pump relay outputs based on state machine
void updatePumpControl() {
  PumpControlMode mode = stateMachine.getPumpControlMode();
  
  switch (mode) {
    case PUMP_NORMAL:
      digitalWrite(PIN_FORCE_PUMP_HK2, LOW);
      digitalWrite(PIN_BLOCK_PUMP_HK2, LOW);
      break;
    case PUMP_BLOCKED:
      digitalWrite(PIN_FORCE_PUMP_HK2, LOW);
      digitalWrite(PIN_BLOCK_PUMP_HK2, HIGH);
      break;
    case PUMP_FORCED:
      digitalWrite(PIN_FORCE_PUMP_HK2, HIGH);
      digitalWrite(PIN_BLOCK_PUMP_HK2, LOW);
      break;
  }
}

// Function to build status log message (used by both Serial and Web)
String buildStatusLog() {
  char buffer[256];  // Increased buffer for new format
  
  // PWM-in status
  char pwmInBuf[10];
  if (pwmDetected) {
    snprintf(pwmInBuf, sizeof(pwmInBuf), "%.1f%%", dutyIn);
  } else {
    strcpy(pwmInBuf, "OFF");
  }
  
  // PWM-out status
  char pwmOutBuf[10];
  if (stateMachine.getCurrentState() == POST_RUN) {
    snprintf(pwmOutBuf, sizeof(pwmOutBuf), "%d%%", PWM_OUTPUT_DUTY_POSTRUN);
  } else {
    strcpy(pwmOutBuf, "OFF");
  }
  
  // Temperature status
  char tempBuf[15];
  if (tempSensor.isAvailable()) {
    snprintf(tempBuf, sizeof(tempBuf), "%.1f", tempSensor.getTemperature());
  } else {
    strcpy(tempBuf, "--");
  }
  
  // Build complete log line with new format:
  // [YYYY-MM-DD HH:MM:SS] State: <STATE>  PumpHK2: <STATUS>  PWM-in: <X%|OFF>  PWM-out: <X%|OFF>  Flow: <XX.X|-->°C
  snprintf(buffer, sizeof(buffer), 
           "[%s] State: %s  PumpHK2: %s  PWM-in: %s  PWM-out: %s  Flow: %s°C",
           getTimestamp().c_str(), 
           stateMachine.getStateString().c_str(),
           stateMachine.getPumpStatusString().c_str(),
           pwmInBuf, 
           pwmOutBuf, 
           tempBuf);
  
  return String(buffer);
}

// Function to log current data line to Serial
void logDataLine() {
  String line = buildStatusLog();
  Serial.println(line);
  addWebLogLine(line);
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);
  Serial.println("\n\n=== SYSTEM STARTUP ===");

  pinMode(PIN_PWM_OUT, OUTPUT);         // PWM output
  pinMode(PIN_FORCE_PUMP_HK2, OUTPUT);  // Force pump relay (D7)
  pinMode(PIN_BLOCK_PUMP_HK2, OUTPUT);  // Block pump relay (D8)
  pinMode(LED_PIN, OUTPUT);

  if (DEBUG_MODE) {
    Serial.print("DEBUG: PIN_FORCE_PUMP_HK2 = ");
    Serial.println(PIN_FORCE_PUMP_HK2);
    Serial.print("DEBUG: PIN_BLOCK_PUMP_HK2 = ");
    Serial.println(PIN_BLOCK_PUMP_HK2);
  }

  digitalWrite(LED_PIN, LOW);               // LED aan = script draait
  digitalWrite(PIN_FORCE_PUMP_HK2, LOW);    // Force pump starts OFF
  digitalWrite(PIN_BLOCK_PUMP_HK2, LOW);    // Block pump starts OFF

  // PWM op D1
  analogWriteRange(1023);
  // PWM output starts inactive (only activates during post-run timer)
  digitalWrite(PIN_PWM_OUT, LOW);
  if (DEBUG_MODE) {
    Serial.println("DEBUG: PWM output initialized (OFF)");
    
    // Test PWM output for 2 seconds
    Serial.println("DEBUG: Testing PWM output for 2 seconds...");
    analogWriteFreq(PWM_OUTPUT_FREQ_POSTRUN);
    int pwmTestValue = (PWM_OUTPUT_DUTY_POSTRUN * 1023) / 100;
    analogWrite(PIN_PWM_OUT, pwmTestValue); // Configured duty
    delay(PWM_TEST_DURATION);
    analogWrite(PIN_PWM_OUT, 0);
    digitalWrite(PIN_PWM_OUT, LOW);
    Serial.println("DEBUG: PWM test complete");
  }

  // Initialize PWM detection on D6
  initPWMDetector(PIN_PWM_IN);

  // Initialize state machine
  stateMachine.begin();
  stateMachine.setEventCallback(onStateMachineEvent);
  Serial.println("State machine initialized");

  // Initialize temperature sensor
  tempSensor.begin();
  Serial.print("Temperature sensor initialized: ");
  Serial.println(tempSensor.getStateString());

  // Initialize WiFi connection
  initWiFi(WIFI_SSID, WIFI_PW);

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Webserver gestart op poort 80");
  
  // Configure NTP for time synchronization
  configTime(1 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("NTP gestart, wachtend op tijdsynchronisatie...");
}

// -----------------------------
// Loop
// -----------------------------
void loop() {
  server.handleClient();

  // Maintain WiFi connection
  updateWiFi();

  // Periodically retry NTP sync (non-blocking)
  time_t now = time(nullptr);
  bool timeValid = (now >= 1000000000);  // Time synced if after ~2001
  unsigned long currentMillis = millis();
  unsigned long resyncInterval = timeValid ? NTP_RESYNC_INTERVAL_VALID : NTP_RESYNC_INTERVAL;
  
  if (currentMillis - lastNTPSyncAttempt >= resyncInterval) {
    configTime(1 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    lastNTPSyncAttempt = currentMillis;
    if (DEBUG_MODE) {
      Serial.print("DEBUG: NTP resync poging (");
      Serial.print(timeValid ? "geldig, uur-interval" : "ongeldig, minuut-interval");
      Serial.println(")");
    }
  }

  // Update PWM measurements from interrupt data
  updatePWMDetection();

  // Update temperature sensor (non-blocking)
  tempSensor.update();

  // Update state machine with current conditions
  stateMachine.update(pwmDetected, dutyIn, tempSensor.getTemperature(), tempSensor.isAvailable());

  // Update pump relay outputs
  updatePumpControl();

  // Control PWM output based on state machine
  // PWM output is only active during POST_RUN state
  if (stateMachine.getCurrentState() == POST_RUN) {
    setOutputPWM(PWM_OUTPUT_FREQ_POSTRUN, PWM_OUTPUT_DUTY_POSTRUN);
  } else {
    setOutputPWM(0, 0);  // PWM off
  }

  // LED gedrag based on state machine
  unsigned long currentTime = millis();
  unsigned long blinkInterval;
  HeatPumpState currentState = stateMachine.getCurrentState();
  
  if (currentState == POST_RUN) {
    // Post-run timer active: fast blink
    blinkInterval = LED_BLINK_FAST;
  } else if (currentState != STANDBY) {
    // Any active state (STARTUP, HOT_WATER, HEATING, COOLING): slow blink
    blinkInterval = LED_BLINK_SLOW;
  } else {
    // Standby: short flashes
    blinkInterval = LED_BLINK_STANDBY;
  }

  if (currentTime - lastBlink >= blinkInterval) {
    lastBlink = currentTime;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);  // active LOW
  }

  // Logging: First log immediately, then every 30 seconds
  if (!firstLogSent || (currentTime - lastLog >= LOG_INTERVAL)) {
    if (!firstLogSent) {
      firstLogSent = true;
    }
    lastLog = currentTime;
    
    logDataLine();
  }
}

// Warn once when heap drops below threshold; reset warning when it recovers
void checkHeapAndWarn() {
  uint32_t freeHeap = ESP.getFreeHeap();
  if (!lowHeapWarned && freeHeap < HEAP_WARNING_THRESHOLD) {
    String warnMsg = "[" + getTimestamp() + "] !!! Low heap: " + String(freeHeap) + " bytes remaining !!!";
    Serial.println(warnMsg);
    webLogBuffer[webLogIndex] = warnMsg;
    webLogIndex = (webLogIndex + 1) % WEB_LOG_BUFFER_SIZE;
    if (webLogCount < WEB_LOG_BUFFER_SIZE) {
      webLogCount++;
    }
    lowHeapWarned = true;
  } else if (lowHeapWarned && freeHeap > (HEAP_WARNING_THRESHOLD + 2000)) {
    // Hysteresis: clear warning state after recovery
    lowHeapWarned = false;
  }
}