// pwm D6 input tester

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include "Config.h"
#include "PWMDetector.h"
#include "WiFiManager.h"
#include "PostRunTimerManager.h"

// ===== FUNCTIONALITY OVERVIEW =====
// This application monitors a PWM signal on D6 (from external heat pump) and controls
// a PWM output on D1. It implements the following features:
//
// 1. PWM INPUT DETECTION (D6):
//    - Detects PWM signal via interrupt-based edge detection with debouncing
//    - Measures duty cycle and frequency (100-150 Hz expected)
//    - DC level detection: 100% duty = constant HIGH (valid signal), 0% duty = constant LOW (no signal)
//    - Stable measurement without blocking the main loop
//    - Config: PWM_DEBOUNCE_TIME, PWM_DETECTION_TIMEOUT
//
// 2. PWM OUTPUT CONTROL (D1):
//    - HEATPUMP ACTIVE / STANDBY: No output (PWM inactive)
//    - POST-RUN TIMER ACTIVE: Outputs fixed PWM with configured frequency and duty cycle
//    - Config: PWM_OUTPUT_FREQ_POSTRUN, PWM_OUTPUT_DUTY_POSTRUN
//
// 3. POST-RUN TIMER:
//    - Auto-starts when input PWM is lost
//    - Duration: Configured duration (see POSTRUN_TIMER_DURATION)
//    - Output: Configured frequency and duty cycle
//    - Auto-stops when input PWM returns
//    - Activates pump on D7 during post-run timer
//    - Config: POSTRUN_TIMER_DURATION
//
// 4. PUMP CONTROL (D7):
//    - Normally LOW (OFF)
//    - Goes HIGH (ON) during post-run timer
//
// 5. LED INDICATOR (Built-in LED):
//    - STANDBY: Short flashes (no PWM-in, no post-run)
//    - PWM-IN ACTIVE: Slow blink
//    - POST-RUN TIMER ACTIVE: Fast blink
//    - Config: LED_BLINK_STANDBY, LED_BLINK_SLOW, LED_BLINK_FAST
//
// 6. WIFI & LOGGING:
//    - WiFi connection with auto-reconnect at configured interval (non-blocking)
//    - NTP time synchronization for accurate timestamps
//    - Initial NTP sync at startup, periodic resync (1 min if invalid, 1 hour if valid)
//    - Serial logging at configured interval with format:
//      [YYYY-MM-DD HH:MM:SS] PWM-in: X%/OFF  PWM-out: X%/OFF  Pump-HK2: ON/OFF  Status: ...
//    - Config: WIFI_CONNECTION_TIMEOUT, WIFI_RECONNECT_INTERVAL, NTP_RESYNC_INTERVAL, 
//              NTP_RESYNC_INTERVAL_VALID, LOG_INTERVAL
//
// 7. WEB SERVER:
//    - HTTP server on port 80
//    - Displays current PWM, frequency, and status on root path "/"
//
// ===== CONFIGURATION =====
// All configurable parameters are centralized in include/Config.h
// Edit Config.h to modify behavior (pin mappings, timings, WiFi credentials, etc.)

ESP8266WebServer server(80);

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

// -----------------------------
// Webpagina
// -----------------------------
void handleRoot() {
  String html =
    "<html><head><meta http-equiv='refresh' content='" + String(WEBSERVER_REFRESH_INTERVAL) + "' />"
    "<style>body{font-family:monospace;margin:20px;font-size:12px;} pre{background:#f0f0f0;padding:10px;max-height:600px;overflow-y:auto;} .event{color:blue;margin:10px 0;}</style>"
    "</head><body>"
    "<h2>PWM Monitor</h2>";
  
  // Show log buffer
  html += "<pre>";
  if (webLogCount == 0) {
    html += "No log entries yet...";
  } else {
    // Show all log entries in order
    int startIdx = (webLogCount < WEB_LOG_BUFFER_SIZE) ? 0 : webLogIndex;
    for (int i = 0; i < webLogCount; i++) {
      int idx = (startIdx + i) % WEB_LOG_BUFFER_SIZE;
      html += webLogBuffer[idx] + "\n";
    }
  }
  html += "</pre>";

  server.send(200, "text/html; charset=utf-8", html);
}

// WiFi connectie
// Opgemerkt: WiFi management is nu in WiFiManager.cpp

// Function to set output PWM parameters
void setOutputPWM(int frequency, int duty) {
  if (DEBUG_MODE) {
    Serial.print("DEBUG: setOutputPWM(");
    Serial.print(frequency);
    Serial.print("Hz, ");
    Serial.print(duty);
    Serial.println("%)");
  }
  
  if (duty == 0) {
    // Stop PWM completely
    analogWrite(PIN_PWM_OUT, 0);
    digitalWrite(PIN_PWM_OUT, LOW);
    if (DEBUG_MODE) {
      Serial.println("DEBUG: PWM stopped (pin set to LOW)");
    }
  } else {
    // Set PWM frequency and duty cycle
    analogWriteFreq(frequency);
    int pwmValue = (duty * 1023) / 100;
    analogWrite(PIN_PWM_OUT, pwmValue);
    if (DEBUG_MODE) {
      Serial.print("DEBUG: PWM active - analogWrite value = ");
      Serial.println(pwmValue);
    }
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

  webLogBuffer[webLogIndex] = line;
  webLogIndex = (webLogIndex + 1) % WEB_LOG_BUFFER_SIZE;
  if (webLogCount < WEB_LOG_BUFFER_SIZE) {
    webLogCount++;
  }
}

// Function to build status log message (used by both Serial and Web)
String buildStatusLog() {
  // PWM-in status
  String pwmInStatus = pwmDetected ? String(dutyIn, 1) + "%" : "OFF";
  
  // PWM-out status
  String pwmOutStatus;
  if (isPostRunTimerActive()) {
    pwmOutStatus = String(PWM_OUTPUT_DUTY_POSTRUN) + "%";
  } else {
    pwmOutStatus = "OFF";
  }
  
  // Pump status
  String pumpStatus = isPostRunTimerActive() ? "ON" : "OFF";
  
  // System status
  String statusString;
  if (isPostRunTimerActive()) {
    statusString = "POST-RUN TIMER: " + getPostRunTimerTimeString() + " min";
  } else if (pwmDetected) {
    statusString = "HEATPUMP ACTIVE";
  } else {
    statusString = "STANDBY";
  }
  
  String timestamp = getTimestamp();
  return "[" + timestamp + "] PWM-in: " + pwmInStatus + 
         "  PWM-out: " + pwmOutStatus + 
         "  Pump-HK2: " + pumpStatus + 
         "  Status: " + statusString;
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

  pinMode(PIN_PWM_OUT, OUTPUT);      // PWM bron
  pinMode(PIN_PUMP, OUTPUT);         // Pump HK2 control
  pinMode(LED_PIN, OUTPUT);

  if (DEBUG_MODE) {
    Serial.print("DEBUG: PIN_PUMP = ");
    Serial.println(PIN_PUMP);
  }

  digitalWrite(LED_PIN, LOW);        // LED aan = script draait
  digitalWrite(PIN_PUMP, LOW);       // Pump starts OFF

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

  // Initialize post-run timer manager
  initPostRunTimer(POSTRUN_TIMER_DURATION);
  setPostRunTimerPumpPin(PIN_PUMP);

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

  // Handle post-run timer logic based on PWM-in state changes
  if (pwmStateChanged()) {
    if (pwmDetected) {
      // PWM-in became active: stop post-run timer
      stopPostRunTimer();
      lastStatusChange = "[" + getTimestamp() + "] === PWM-in changed to ON. Post-Run Timer cancelled ===";
      Serial.println(lastStatusChange);
      addWebLogLine(lastStatusChange);
      logDataLine();
    } else {
      // PWM-in was lost: start post-run timer
      float timerMinutes = getPostRunTimerDuration() / 60000.0;
      char msg[120];
      snprintf(msg, sizeof(msg), "[%s] === PWM-in changed to OFF: Post-Run Timer started for %.1f minutes ===", 
               getTimestamp().c_str(), timerMinutes);
      lastStatusChange = msg;
      Serial.println(msg);
      addWebLogLine(lastStatusChange);
      startPostRunTimer();
      logDataLine();
    }
  }

  // Update post-run timer
  bool wasPostRunTimerActive = isPostRunTimerActive();
  updatePostRunTimer();
  
  if (wasPostRunTimerActive && !isPostRunTimerActive()) {
    lastStatusChange = "[" + getTimestamp() + "] === Post-Run Timer finished. Entering STANDBY mode ===";
    Serial.println(lastStatusChange);
    addWebLogLine(lastStatusChange);
    logDataLine();
  }

  // LED gedrag: 3 verschillende snelheden
  unsigned long currentTime = millis();
  unsigned long blinkInterval;
  
  if (isPostRunTimerActive()) {
    // Post-run timer active: fast blink
    blinkInterval = LED_BLINK_FAST;
  } else if (pwmDetected) {
    // PWM-in active: slow blink
    blinkInterval = LED_BLINK_SLOW;
  } else {
    // Standby (no PWM-in, no post-run): short flashes
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