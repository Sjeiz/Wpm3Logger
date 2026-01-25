// Debug flag: set true to show debug messages in the web log
bool DEBUG_TO_WEB = false;
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
#define PIN_FLOWTEMP_IN D2      // GPIO4  - Flow temperature input (with pull-up)
#define PIN_PWM_OUT D5          // GPIO14 - PWM output
#define PIN_PWM_IN D6           // GPIO12 - PWM input (no pull-up)
#define PIN_PUMP_FORCED D7      // GPIO13 - Forces pump HK2 ON during post-run timer
#define PIN_PUMP_BLOCKED D8     // GPIO15 - Blocks pump HK2 during defrost

// OneWire and DS18B20 temperature sensor on D2
OneWire oneWire(PIN_FLOWTEMP_IN);
DallasTemperature tempSensor(&oneWire);
float flowTemp = -127.0;  // -127 = sensor error/not available
float pwmInDutyCycle = 0.0;  // PWM-in duty cycle percentage (not yet implemented)

// Modbus client
ModbusIP mb;

// Webserver
ESP8266WebServer server(80);

// Log buffer (circular buffer for last 60 lines)
#define LOG_BUFFER_SIZE 60
String logBuffer[LOG_BUFFER_SIZE];
int logIndex = 0;
int logCount = 0;

// Timing
unsigned long lastReadTime = 0;
unsigned long lastWifiCheckTime = 0;
unsigned long lastDetailLogTime = 0;
unsigned long postRunTimerEnd = 0;
bool postRunActive = false;
uint8_t wifiCheckCounter = 0;
int logsSinceStateChange = 0;  // Counter for lines since last state change

// NTP configuration
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 3600;        // UTC+1 (Netherlands winter)
const int DAYLIGHT_OFFSET_SEC = 3600;    // +1 hour for daylight saving time
bool timeInitialized = false;

// State machine
enum State {
    STANDBY,
    DEFROSTING,
    COOLING,
    HOT_WATER,
    HEATING,
    POST_RUN,
    ERROR
  };

struct StateMachine {
    bool updateStateMachine(State newState) {
      if (newState != stateMachine.currentState) {
        unsigned long timeInPreviousState = (millis() - stateMachine.stateEnteredAt) / 1000;
        String timeString;
        if (timeInPreviousState < 60) {
          if (timeInPreviousState < 10) timeString = "0";
          timeString += String(timeInPreviousState) + " sec";
        } else if (timeInPreviousState < 3600) {
          unsigned long minutes = timeInPreviousState / 60;
          unsigned long seconds = timeInPreviousState % 60;
          if (minutes < 10) timeString = "0";
          timeString += String(minutes) + ":";
          if (seconds < 10) timeString += "0";
          timeString += String(seconds) + " min";
        } else {
          unsigned long hours = timeInPreviousState / 3600;
          unsigned long minutes = (timeInPreviousState % 3600) / 60;
          if (hours < 10) timeString = "0";
          timeString += String(hours) + ":";
          if (minutes < 10) timeString += "0";
          timeString += String(minutes) + " h";
        }

        // Cancel post-run if compressor starts again
        if (postRunActive && newState != STANDBY) {
          postRunActive = false;
          digitalWrite(PIN_PUMP_FORCED, LOW);
          String cancelMsg = "[" + getTimestamp() + "] POST-RUN: Cancelled due to compressor start";
          logMessage(cancelMsg);
        }

        // Start post-run timer when leaving HOT_WATER
        if (stateMachine.currentState == HOT_WATER && newState != HOT_WATER) {
          postRunActive = true;
          postRunTimerEnd = millis() + (20 * 60 * 1000); // 20 minutes
          String timerMsg = "[" + getTimestamp() + "] POST-RUN: Timer started (20 min)";
          logMessage(timerMsg);
        }

        updateGpioOutputs(newState);

        String stateMsg = "[" + getTimestamp() + "] === STATE: ";
        stateMsg += stateToString(stateMachine.currentState);
        stateMsg += " → ";
        stateMsg += stateToString(newState);
        stateMsg += " (";
        stateMsg += timeString;
        stateMsg += " in last state) ===";
        logMessage(stateMsg); // Serial + web buffer

        stateMachine.previousState = stateMachine.currentState;
        stateMachine.currentState = newState;
        stateMachine.stateEnteredAt = millis();
        logsSinceStateChange = 0;
        return true;
      }

      // If state change log has scrolled out of buffer, re-log it
      if (logsSinceStateChange >= LOG_BUFFER_SIZE) {
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
          timeString += String(minutes) + " h";
        }
        String stateMsg = "[" + getTimestamp() + "] === STATE: ";
        stateMsg += stateToString(stateMachine.currentState);
        stateMsg += " (" + timeString + " in current state) ===";
        logMessage(stateMsg); // Serial + web buffer
        logsSinceStateChange = 0;
      }
      return false;
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
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

void logDebugStatusBits(uint16_t statusBits) {
  String debugMsg = "[" + getTimestamp() + "] DEBUG: Modbus status register (2501) = 0x";
  debugMsg += String(statusBits, HEX);
  // Always log debug to serial
  Serial.println(debugMsg);

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

  Serial.println(explanation);

  // Only log debug to web if enabled
  if (DEBUG_TO_WEB) {
    int idx = logIndex % LOG_BUFFER_SIZE;
    logBuffer[idx] = debugMsg;
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
    if (logCount < LOG_BUFFER_SIZE) logCount++;
    idx = logIndex % LOG_BUFFER_SIZE;
    logBuffer[idx] = explanation;
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
    if (logCount < LOG_BUFFER_SIZE) logCount++;
  }
}

void readFlowTemp() {
  tempSensor.requestTemperatures();
  flowTemp = tempSensor.getTempCByIndex(0);
  
  // Error detectie: -127 = sensor error, 85 = power-on default
  if (flowTemp == -127.0 || flowTemp == 85.0) {
    flowTemp = -127.0;  // Markeer als invalid
  }
}
                State newState = determineState(0, postRunActive, true);
                bool stateChanged = updateStateMachine(newState);
                if (stateChanged) {
                  logDebugMessage("State set to ERROR due to Modbus error.");
                }

void logDetailedStatus(bool toWeb) {
  String detailMsg = "[" + getTimestamp() + "] State: ";
  detailMsg += stateToString(stateMachine.currentState);
  // Show error reason if in ERROR state
  if (stateMachine.currentState == ERROR) {
    if (WiFi.status() != WL_CONNECTED) {
      detailMsg += " (WiFi unavailable)";
    } else {
      // Assume Modbus error if WiFi is up
      detailMsg += " (Modbus unavailable)";
    }
  }
  // Show remaining POST-RUN timer after state if in POST_RUN
  if (stateMachine.currentState == POST_RUN && postRunTimerActive) {
    float remainingMin = float(postRunTimerEnd - millis()) / 60000.0;
    if (remainingMin < 0) remainingMin = 0;
    detailMsg += " (";
    detailMsg += String(remainingMin, 1);
    detailMsg += " min)";
  }

  // PumpHK2 status (obv D7 en D8)
  detailMsg += "   PumpHK2: ";
  if (digitalRead(PIN_PUMP_BLOCKED) == HIGH) {
    detailMsg += "BLOCKED";
  } else if (digitalRead(PIN_PUMP_FORCED) == HIGH) {
    detailMsg += "FORCED";
  } else {
    detailMsg += "NORMAL";
  }

  // PWM-in (dummy voor nu)
  detailMsg += "   PWM-in: ";
  detailMsg += String(pwmInDutyCycle, 1) + "%";

  // PWM-out status
  detailMsg += "   PWM-out: ";
  if (postRunTimerActive) {
    detailMsg += String(PWM_DUTY_CYCLE) + "%";
  } else {
    detailMsg += "OFF";
  }

  // Flow temperatuur
  detailMsg += "   FlowTemp: ";
  if (flowTemp != -127.0) {
    detailMsg += String(flowTemp, 1) + "°C";
  } else {
    detailMsg += "--°C";
  }
  
  if (toWeb) {
    // Alleen naar webbuffer, niet naar Serial
    int idx = logIndex % LOG_BUFFER_SIZE;
    logBuffer[idx] = detailMsg;
    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
    if (logCount < LOG_BUFFER_SIZE) logCount++;
  } else {
    Serial.println(detailMsg);  // Alleen naar Serial
  }
}

void updateGpioOutputs(State state) {
    switch (state) {
      case POST_RUN:
        analogWrite(PIN_PWM_OUT, (1023 * PWM_DUTY_CYCLE) / 100);
        digitalWrite(PIN_PUMP_FORCED, HIGH);
        digitalWrite(PIN_PUMP_BLOCKED, LOW);
        break;
      case DEFROSTING:
        analogWrite(PIN_PWM_OUT, 0);
        digitalWrite(PIN_PUMP_FORCED, LOW);
        digitalWrite(PIN_PUMP_BLOCKED, HIGH);
        break;
      case ERROR:
        analogWrite(PIN_PWM_OUT, 0);
        digitalWrite(PIN_PUMP_FORCED, LOW);
        digitalWrite(PIN_PUMP_BLOCKED, LOW);
        break;
      default: // STANDBY, COOLING, HOT_WATER, HEATING
        analogWrite(PIN_PWM_OUT, 0);
        digitalWrite(PIN_PUMP_FORCED, LOW);
        digitalWrite(PIN_PUMP_BLOCKED, LOW);
        break;
    }
}

const char* stateToString(State state) {
  switch(state) {
    case STANDBY: return "STANDBY";
    case DEFROSTING: return "DEFROSTING";
    case COOLING: return "COOLING";
    case HOT_WATER: return "HOT_WATER";
    case HEATING: return "HEATING";
    case POST_RUN: return "POST_RUN";
    case ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}
// Overload determineState to support error argument
State determineState(uint16_t statusBits, bool postRunActive, bool error) {
  if (error) return ERROR;
  bool compressor = statusBits & (1 << 6);
  bool defrost    = statusBits & (1 << 9);
  bool cooling    = statusBits & (1 << 8);
  bool hotWater   = statusBits & (1 << 5);
  bool heating    = statusBits & (1 << 4);

  if (!compressor) {
    if (postRunActive) return POST_RUN;
    return STANDBY;
  }
  if (defrost)  return DEFROSTING;
  if (cooling)  return COOLING;
  if (hotWater) return HOT_WATER;
  if (heating)  return HEATING;
  return STANDBY;
}
  // In ERROR, both outputs LOW (normal)
  if (state == ERROR) {
    digitalWrite(PIN_PUMP_FORCED, LOW);
    digitalWrite(PIN_PUMP_BLOCKED, LOW);
  }

State determineState(uint16_t statusBits, bool postRunActive) {
    bool compressor = statusBits & (1 << 6);
    bool defrost    = statusBits & (1 << 9);
    bool cooling    = statusBits & (1 << 8);
    bool hotWater   = statusBits & (1 << 5);
    bool heating    = statusBits & (1 << 4);

    if (!compressor) {
        if (postRunActive) return POST_RUN;
        return STANDBY;
    }
    if (defrost)  return DEFROSTING;
    if (cooling)  return COOLING;
    if (hotWater) return HOT_WATER;
    if (heating)  return HEATING;
    return STANDBY; // fallback
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
    // 1. Read FlowTemp sensor (DS18B20 on D2)
    readFlowTemp();

    // 2. Check Modbus TCP connection with ISG-web
    if (!mb.isConnected(IPAddress())) {
      Serial.println("Modbus not connected, reconnecting...");
      connectModbus();
      // Do not return; continue to set outputs to safe state
    }

    // 3. Read WPM3 status register 2501 (Modbus address 2500)
    uint16_t res = mb.readHreg(IPAddress(), 2500, nullptr, 1, nullptr, ISG_SLAVE_ID);

    if (res == 0) {
      delay(100);
      uint16_t statusBits = mb.Hreg(2500);

      // Determine and update state
      State newState = determineState(statusBits, postRunActive);
      bool stateChanged = updateStateMachine(newState);

      // Debug logging on state change
      if (stateChanged) {
        logDebugStatusBits(statusBits);
      }

      // Log detailed status to Serial (every 15 seconds)
      logDetailedStatus(false);
    } else {
      logMessage("⚠️ WARNING: Cannot read Modbus register 2500. Setting pump outputs to normal.");
      digitalWrite(PIN_PUMP_FORCED, LOW);   // D7 normal
      digitalWrite(PIN_PUMP_BLOCKED, LOW); // D8 normal
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
    if (postRunActive && millis() >= postRunTimerEnd) {
        postRunActive = false;
        digitalWrite(PIN_PUMP_FORCED, LOW);
        String timerMsg = "[" + getTimestamp() + "] POST-RUN: Timer expired";
        logMessage(timerMsg);
    }

    // Check every 60 seconds if WiFi is still connected
    if (millis() - lastWifiCheckTime >= 60000) {
        lastWifiCheckTime = millis();
        wifiCheckCounter++;

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println();
            Serial.println("WiFi connection lost. Reconnecting...");
            connectWiFi();

            // If WiFi is now connected, reinitialize NTP and Modbus
            if (WiFi.status() == WL_CONNECTED) {
                delay(1000);
                initTime();
                wifiCheckCounter = 0; // Reset counter after reconnect
                delay(1000);
                connectModbus();
            }
        } else if (wifiCheckCounter >= 60) {
            // Every hour (60 × 60 seconds): NTP resync
            Serial.println();
            initTime();
            wifiCheckCounter = 0;
        }
    }

    // Periodic reading (only if WiFi is connected)
    if (WiFi.status() == WL_CONNECTED && millis() - lastReadTime >= READ_INTERVAL) {
        readInputValues();  // Read Modbus register 2501 + FlowTemp sensor
        lastReadTime = millis();
    }

    // Periodic detailed logging to webserver (every 30 seconds)
    if (millis() - lastDetailLogTime >= 30000) {
        logDetailedStatus(true);  // To Serial + Web buffer
        lastDetailLogTime = millis();
    }

    delay(10);
}