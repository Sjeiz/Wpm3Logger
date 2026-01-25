#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ModbusIP_ESP8266.h>
#include <time.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

/*
 * Register 2501 (Modbus address 2500) Bit Mapping:
 * D0: HK1 Pump
 * D1: HK2 Pump
 * D2: HK3 Pump
 * D3: DHW Pump (Domestic Hot Water)
 * D4: HEATING mode
 * D5: HOT_WATER mode
 * D6: COMPRESSOR status
 * D7: PUMP_FORCED - Normal = LOW, HIGH - Forces pump HK2 ON during post-run timer
 * D8: PUMP_BLOCKED - Normal = LOW, =HIGH - Blocks pump HK2 during defrost
 * D9: DEFROSTING mode
 * D10: (unknown)
 * D11: (unknown)
 */

// GPIO pins
#define PIN_FLOWTEMP_IN D2      // GPIO4  - Flow temperature input (met pull-up)
#define PIN_PWM_OUT D5          // GPIO14 - PWM output
#define PIN_PWM_IN D6           // GPIO12 - PWM input (zonder pull-up)
#define PIN_PUMP_FORCED D7      // GPIO13 - Forces pump HK2 ON during post-run timer
#define PIN_PUMP_BLOCKED D8     // GPIO15 - Blocks pump HK2 during defrost

// OneWire en DS18B20 temperatuursensor op D2
OneWire oneWire(PIN_FLOWTEMP_IN);
DallasTemperature tempSensor(&oneWire);
float flowTemp = -127.0;  // -127 = sensor error/niet beschikbaar
float pwmInDutyCycle = 0.0;  // PWM-in duty cycle percentage (dummy, nog te implementeren)

// Modbus client
ModbusIP mb;

// Webserver
ESP8266WebServer server(80);

// Log buffer (circular buffer voor laatste 60 regels)
#define LOG_BUFFER_SIZE 60
String logBuffer[LOG_BUFFER_SIZE];
int logIndex = 0;
int logCount = 0;

// Timing
unsigned long lastReadTime = 0;
unsigned long lastWifiCheckTime = 0;
unsigned long lastDetailLogTime = 0;
unsigned long postRunTimerEnd = 0;
bool postRunTimerActive = false;
uint8_t wifiCheckCounter = 0;
int logsSinceStateChange = 0;  // Teller voor regels sinds laatste state change

// NTP configuratie
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 3600;        // UTC+1 (Nederland winter)
const int DAYLIGHT_OFFSET_SEC = 3600;    // +1 uur voor zomertijd
bool timeInitialized = false;

// State machine
enum WpmState {
  STANDBY,
  HEATING,
  HOT_WATER,
  COOLING,
  DEFROSTING
};

struct StateMachine {
  WpmState currentState;
  WpmState previousState;
  unsigned long stateEnteredAt;
};

StateMachine stateMachine = {STANDBY, STANDBY, 0};

void logMessage(const String& message) {
  // Print naar Serial
  Serial.println(message);
  
  // Voeg toe aan circular buffer
  logBuffer[logIndex] = message;
  logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
  if (logCount < LOG_BUFFER_SIZE) {
    logCount++;
  }
  
  // Teller voor buffer overflow detectie
  logsSinceStateChange++;
}

void logDebugMessage(const String& message) {
  // Altijd naar Serial
  Serial.println(message);
  
  // Alleen naar webserver buffer als DEBUG_ENABLED=true
  if (DEBUG_ENABLED) {
    logBuffer[logIndex] = message;
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
    if (logCount < LOG_BUFFER_SIZE) {
      logCount++;
    }
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>WPM3 Logger</title>";
  html += "<meta http-equiv='refresh' content='30'>";
  html += "<style>";
  html += "body { font-family: 'Courier New', monospace; background: #1e1e1e; color: #d4d4d4; margin: 20px; }";
  html += "h1 { color: #4ec9b0; }";
  html += ".log { background: #252526; padding: 10px; border-radius: 5px; overflow-x: auto; }";
  html += ".log pre { margin: 0; white-space: pre-wrap; word-wrap: break-word; }";
  html += ".info { color: #888; margin-bottom: 20px; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>🔥 WPM3 Modbus Logger</h1>";
  html += "<div class='info'>";
  html += "IP: " + WiFi.localIP().toString() + " | ";
  html += "Uptime: " + String(millis() / 1000) + "s | ";
  html += "Auto-refresh: 30s";
  html += "</div>";
  html += "<div class='log'><pre>";
  
  // Toon laatste 60 regels, nieuwste bovenaan
  int startIdx = (logIndex - 1 + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
  for (int i = 0; i < logCount; i++) {
    int idx = (startIdx - i + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
    html += logBuffer[idx] + "\n";
  }
  
  html += "</pre></div>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void initTime() {
  // Controleer WiFi verbinding
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi niet verbonden! NTP synchronisatie overgeslagen.");
    return;
  }
  
  Serial.print("Synchronizing NTP server '");
  Serial.print(NTP_SERVER);
  Serial.print("'...");
  
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  // Wacht tot tijd is gesynchroniseerd (max 10 seconden)
  int timeout = 0;
  time_t now = time(nullptr);
  while (now < 24 * 3600 && timeout < 20) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    timeout++;
  }
  
  Serial.println();
  
  if (now > 24 * 3600) {
    timeInitialized = true;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    String ntpMsg = "✅ NTP synchronized: ";
    ntpMsg += asctime(&timeinfo);
    ntpMsg.trim();
    logMessage(ntpMsg);
  } else {
    logMessage("❌ NTP synchronization failed!");
  }
}

String getTimestamp() {
  char buffer[20];
  
  if (timeInitialized) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  } else {
    // Fallback: gebruik uptime
    unsigned long totalSeconds = millis() / 1000;
    unsigned long hours = (totalSeconds % 86400) / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;
    
    snprintf(buffer, sizeof(buffer), "UPTIME %02lu:%02lu:%02lu",
             hours, minutes, seconds);
  }
  
  return String(buffer);
}

void logDebugStatusBits(uint16_t statusBits) {
  String debugMsg = "[" + getTimestamp() + "] DEBUG: Modbus status register (2501) = 0x";
  debugMsg += String(statusBits, HEX);
  logDebugMessage(debugMsg);
  
  // Verklaar welke bits actief zijn
  String explanation = "[" + getTimestamp() + "] DEBUG: Active bits: ";
  bool hasActiveBits = false;
  
  if (statusBits & (1 << 0)) { explanation += "HK1_Pump "; hasActiveBits = true; }
  if (statusBits & (1 << 1)) { explanation += "HK2_Pump "; hasActiveBits = true; }
  if (statusBits & (1 << 2)) { explanation += "HK3_Pump "; hasActiveBits = true; }
  if (statusBits & (1 << 3)) { explanation += "DHW_Pump "; hasActiveBits = true; }
  if (statusBits & (1 << 4)) { explanation += "HEATING "; hasActiveBits = true; }
  if (statusBits & (1 << 5)) { explanation += "HOT_WATER "; hasActiveBits = true; }
  if (statusBits & (1 << 6)) { explanation += "COMPRESSOR "; hasActiveBits = true; }
  if (statusBits & (1 << 7)) { explanation += "Aux_Heater "; hasActiveBits = true; }
  if (statusBits & (1 << 8)) { explanation += "COOLING "; hasActiveBits = true; }
  if (statusBits & (1 << 9)) { explanation += "DEFROSTING "; hasActiveBits = true; }
  if (statusBits & (1 << 10)) { explanation += "B10 "; hasActiveBits = true; }
  if (statusBits & (1 << 11)) { explanation += "B11 "; hasActiveBits = true; }
  
  if (!hasActiveBits) {
    explanation += "(none)";
  }
  
  logDebugMessage(explanation);
}

void readFlowTemp() {
  tempSensor.requestTemperatures();
  flowTemp = tempSensor.getTempCByIndex(0);
  
  // Error detectie: -127 = sensor error, 85 = power-on default
  if (flowTemp == -127.0 || flowTemp == 85.0) {
    flowTemp = -127.0;  // Markeer als invalid
  }
}

void logDetailedStatus(bool toWeb) {
  String detailMsg = "[" + getTimestamp() + "] State: ";
  detailMsg += stateToString(stateMachine.currentState);
  
  // PumpHK2 status (obv D7 en D8)
  detailMsg += "  PumpHK2: ";
  if (digitalRead(PIN_PUMP_BLOCKED) == HIGH) {
    detailMsg += "BLOCKED";
  } else if (digitalRead(PIN_PUMP_FORCED) == HIGH) {
    detailMsg += "FORCED";
  } else {
    detailMsg += "NORMAL";
  }
  
  // PWM-in (dummy voor nu)
  detailMsg += "  PWM-in: ";
  detailMsg += String(pwmInDutyCycle, 1) + "%";
  
  // PWM-out status
  detailMsg += "  PWM-out: ";
  if (postRunTimerActive) {
    detailMsg += String(PWM_DUTY_CYCLE) + "%";
  } else {
    detailMsg += "OFF";
  }
  
  // Flow temperatuur
  detailMsg += "  FlowTemp: ";
  if (flowTemp != -127.0) {
    detailMsg += String(flowTemp, 1) + "°C";
  } else {
    detailMsg += "ERR";
  }
  
  if (toWeb) {
    logMessage(detailMsg);  // Naar Serial + Web buffer
  } else {
    Serial.println(detailMsg);  // Alleen naar Serial
  }
}

void updateGpioOutputs(WpmState state) {
  // D5 (PWM_OUT): PWM signaal tijdens post-run timer, anders LOW
  if (postRunTimerActive) {
    // PWM: 150Hz, 30% duty cycle
    // ESP8266 PWM range: 0-1023 (10-bit)
    uint16_t pwmValue = (1023 * PWM_DUTY_CYCLE) / 100;
    analogWrite(PIN_PWM_OUT, pwmValue);
  } else {
    analogWrite(PIN_PWM_OUT, 0);
  }
  
  // D7 (PUMP_FORCED): HIGH tijdens post-run timer, anders LOW
  if (postRunTimerActive) {
    digitalWrite(PIN_PUMP_FORCED, HIGH);
  } else {
    digitalWrite(PIN_PUMP_FORCED, LOW);
  }
  
  // D8 (PUMP_BLOCKED): HIGH tijdens DEFROST, anders LOW
  if (state == DEFROSTING) {
    digitalWrite(PIN_PUMP_BLOCKED, HIGH);
  } else {
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
  }
}

const char* stateToString(WpmState state) {
  switch(state) {
    case STANDBY: return "STANDBY";
    case HEATING: return "HEATING";
    case HOT_WATER: return "HOT_WATER";
    case COOLING: return "COOLING";
    case DEFROSTING: return "DEFROSTING";
    default: return "UNKNOWN";
  }
}

WpmState determineState(uint16_t statusBits) {
  // Check compressor bit (B6)
  bool compressorRunning = (statusBits & (1 << 6)) != 0;
  
  // Als compressor niet loopt → altijd STANDBY
  if (!compressorRunning) {
    return STANDBY;
  }
  
  // Compressor loopt, bepaal operationele state
  // Priority: defrost > cooling > hot_water > heating
  if (statusBits & (1 << 9)) return DEFROSTING;  // B9: Abtaubetrieb
  if (statusBits & (1 << 8)) return COOLING;     // B8: Kuehlbetrieb
  if (statusBits & (1 << 5)) return HOT_WATER;   // B5: Warmwasser
  if (statusBits & (1 << 4)) return HEATING;     // B4: Heizen
  
  return STANDBY;
}

bool updateStateMachine(WpmState newState) {
  if (newState != stateMachine.currentState) {
    unsigned long timeInPreviousState = (millis() - stateMachine.stateEnteredAt) / 1000;
    
    // Formatteer tijd in state (seconden, minuten of uren)
    String timeString;
    if (timeInPreviousState < 60) {
      // Minder dan 1 minuut: "ss sec"
      if (timeInPreviousState < 10) timeString = "0";
      timeString += String(timeInPreviousState) + " sec";
    } else if (timeInPreviousState < 3600) {
      // Minder dan 1 uur: "mm:ss min"
      unsigned long minutes = timeInPreviousState / 60;
      unsigned long seconds = timeInPreviousState % 60;
      if (minutes < 10) timeString = "0";
      timeString += String(minutes) + ":";
      if (seconds < 10) timeString += "0";
      timeString += String(seconds) + " min";
    } else {
      // 1 uur of meer: "hh:mm uur"
      unsigned long hours = timeInPreviousState / 3600;
      unsigned long minutes = (timeInPreviousState % 3600) / 60;
      if (hours < 10) timeString = "0";
      timeString += String(hours) + ":";
      if (minutes < 10) timeString += "0";
      timeString += String(minutes) + " uur";
    }
    
    // Annuleer post-run timer als compressor opnieuw start
    if (postRunTimerActive && newState != STANDBY) {
      postRunTimerActive = false;
      digitalWrite(PIN_PUMP_FORCED, LOW);
      
      String cancelMsg = "[" + getTimestamp() + "] POST-RUN: Cancelled due to compressor start";
      logMessage(cancelMsg);
    }
    
    // Detecteer wanneer we HOT_WATER verlaten → start post-run timer
    if (stateMachine.currentState == HOT_WATER && newState != HOT_WATER) {
      postRunTimerActive = true;
      postRunTimerEnd = millis() + (20 * 60 * 1000); // 20 minuten
      
      String timerMsg = "[" + getTimestamp() + "] POST-RUN: Timer started (20 min)";
      logMessage(timerMsg);
    }
    
    // Update GPIO outputs op basis van nieuwe state
    updateGpioOutputs(newState);
    
    // State change log (currentState bevat nog de OUDE state op dit moment)
    String stateMsg = "[" + getTimestamp() + "] === STATE: ";
    stateMsg += stateToString(stateMachine.currentState);  // Oude state
    stateMsg += " → ";
    stateMsg += stateToString(newState);  // Nieuwe state
    stateMsg += " (";
    stateMsg += timeString;  // Tijd in vorige state (geformatteerd)
    stateMsg += " in last state) ===";
    logMessage(stateMsg);
    
    // Nu pas de state machine updaten
    stateMachine.previousState = stateMachine.currentState;
    stateMachine.currentState = newState;
    stateMachine.stateEnteredAt = millis();
    
    // Reset teller bij nieuwe state change
    logsSinceStateChange = 0;
    
    return true;
  }
  
  // Controleer of state change regel uit buffer is verdwenen (>= 60 regels gelogd)
  if (logsSinceStateChange >= LOG_BUFFER_SIZE) {
    // Log state opnieuw zodat deze zichtbaar blijft in weblog
    unsigned long timeInCurrentState = (millis() - stateMachine.stateEnteredAt) / 1000;
    String timeString;
    if (timeInCurrentState < 60) {
      if (timeInCurrentState < 10) timeString = "0";
      timeString += String(timeInCurrentState) + " sec";
    } else if (timeInCurrentState < 3600) {
      unsigned long minutes = timeInCurrentState / 60;
      unsigned long seconds = timeInCurrentState % 60;
      if (minutes < 10) timeString = "0";
      timeString += String(minutes) + ":";
      if (seconds < 10) timeString += "0";
      timeString += String(seconds) + " min";
    } else {
      unsigned long hours = timeInCurrentState / 3600;
      unsigned long minutes = (timeInCurrentState % 3600) / 60;
      if (hours < 10) timeString = "0";
      timeString += String(hours) + ":";
      if (minutes < 10) timeString += "0";
      timeString += String(minutes) + " uur";
    }
    
    String stateMsg = "[" + getTimestamp() + "] === STATE: ";
    stateMsg += stateToString(stateMachine.currentState);
    stateMsg += " (" + timeString + " in current state) ===";
    logMessage(stateMsg);
  }
  
  return false;
}

void connectWiFi() {
  Serial.print("Connecting WiFi '");
  Serial.print(WIFI_SSID);
  Serial.print("'...");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 60) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    logMessage("✅ WiFi connection established: IP address " + WiFi.localIP().toString());
  } else {
    logMessage("❌ WiFi connection failed!");
  }
}

void connectModbus() {
  // Controleer WiFi verbinding
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi niet verbonden! Modbus verbinding overgeslagen.");
    return;
  }
  
  Serial.print("Verbinden met ISG-web op ");
  Serial.print(ISG_HOST);
  Serial.println("...");
  
  IPAddress isgIP;
  if (WiFi.hostByName(ISG_HOST, isgIP)) {
    Serial.print("ISG-web IP: ");
    Serial.println(isgIP);
    
    mb.client();
    if (mb.connect(isgIP, ISG_PORT)) {
      logMessage("✅ Modbus TCP verbonden!");
    } else {
      logMessage("❌ Modbus verbinding mislukt!");
    }
  } else {
    logMessage("❌ DNS lookup mislukt!");
  }
}

void readInputValues() {
  // 1. Lees FlowTemp sensor (DS18B20 op D2)
  readFlowTemp();
  
  // 2. Controleer Modbus TCP verbinding met ISG-web
  if (!mb.isConnected(IPAddress())) {
    Serial.println("Modbus niet verbonden, opnieuw verbinden...");
    connectModbus();
    return;
  }
  
  // 3. Lees WPM3 status register 2501 (Modbus address 2500)
  uint16_t res = mb.readHreg(IPAddress(), 2500, nullptr, 1, nullptr, ISG_SLAVE_ID);
  
  if (res == 0) {
    delay(100);
    uint16_t statusBits = mb.Hreg(2500);
    
    // Bepaal en update state
    WpmState newState = determineState(statusBits);
    bool stateChanged = updateStateMachine(newState);
    
    // Debug logging bij state change
    if (stateChanged) {
      logDebugStatusBits(statusBits);
    }
    
    // Log detailed status naar Serial (elke 15 seconden)
    logDetailedStatus(false);
  } else {
    logMessage("❌ FOUT: Kan register 2500 niet lezen");
  }
}

void setup() {
  // Configureer PWM frequentie voor D5
  analogWriteFreq(PWM_FREQUENCY);
  
  // Initialiseer GPIO pins
  pinMode(PIN_FLOWTEMP_IN, INPUT_PULLUP);  // D2: Flow temp input met pull-up
  pinMode(PIN_PWM_OUT, OUTPUT);            // D5: PWM output
  pinMode(PIN_PWM_IN, INPUT);              // D6: PWM input zonder pull-up
  pinMode(PIN_PUMP_FORCED, OUTPUT);        // D7: Pump forced output
  pinMode(PIN_PUMP_BLOCKED, OUTPUT);       // D8: Pump blocked output
  
  analogWrite(PIN_PWM_OUT, 0);             // PWM uit bij startup
  digitalWrite(PIN_PUMP_FORCED, LOW);
  digitalWrite(PIN_PUMP_BLOCKED, LOW);
  
  // Initialiseer DS18B20 temperatuursensor op D2
  tempSensor.begin();
  tempSensor.setResolution(12);  // 12-bit resolutie (0.0625°C, ~750ms conversie)
  tempSensor.setWaitForConversion(false);  // Async temperatuur conversie
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n╔════════════════════════════════════╗");
  Serial.println("║   WPM3 Modbus Register 2500       ║");
  Serial.println("║   Status Reader                    ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  // Stap 1: WiFi verbinding
  connectWiFi();
  
  // Als WiFi verbonden is, initialiseer NTP en Modbus
  if (WiFi.status() == WL_CONNECTED) {
    delay(1000);
    
    Serial.println();
    // Stap 2: NTP tijd synchronisatie (vereist WiFi)
    initTime();
    delay(1000);
    
    // Stap 3: Modbus verbinding (vereist WiFi)
    connectModbus();
    
    // Stap 4: Webserver opstarten
    server.on("/", handleRoot);
    server.begin();
    Serial.println();
    Serial.print("✅ Webserver gestart op http://");
    Serial.println(WiFi.localIP());
    
    Serial.println("\n✅ Setup voltooid!");
    Serial.println("Eerste uitlezing over 5 seconden...\n");
  } else {
    Serial.println("\nSetup continues without WiFi. Will retry every 60 seconds...\n");
  }
  
  lastReadTime = millis() - READ_INTERVAL + 5000;
  lastWifiCheckTime = millis();
}

void loop() {
  mb.task();
  server.handleClient();
  
  // Check post-run timer
  if (postRunTimerActive && millis() >= postRunTimerEnd) {
    postRunTimerActive = false;
    digitalWrite(PIN_PUMP_FORCED, LOW);
    
    String timerMsg = "[" + getTimestamp() + "] POST-RUN: Timer expired";
    logMessage(timerMsg);
  }
  
  // Check elke 60 seconden of WiFi nog verbonden is
  if (millis() - lastWifiCheckTime >= 60000) {
    lastWifiCheckTime = millis();
    wifiCheckCounter++;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println();
      Serial.println("WiFi connection lost. Reconnecting...");
      connectWiFi();
      
      // Als WiFi nu verbonden is, herinitialiseer NTP en Modbus
      if (WiFi.status() == WL_CONNECTED) {
        delay(1000);
        initTime();
        wifiCheckCounter = 0; // Reset teller na reconnect
        delay(1000);
        connectModbus();
      }
    } else if (wifiCheckCounter >= 60) {
      // Elk uur (60 × 60 seconden): NTP resync
      Serial.println();
      initTime();
      wifiCheckCounter = 0;
    }
  }
  
  // Periodiek uitlezen (alleen als WiFi verbonden is)
  if (WiFi.status() == WL_CONNECTED && millis() - lastReadTime >= READ_INTERVAL) {
    readInputValues();  // Lees Modbus register 2501 + FlowTemp sensor
    lastReadTime = millis();
  }
  
  // Periodiek detail logging naar webserver (elke 30 seconden)
  if (millis() - lastDetailLogTime >= 30000) {
    logDetailedStatus(true);  // Naar Serial + Web buffer
    lastDetailLogTime = millis();
  }
  
  delay(10);
}