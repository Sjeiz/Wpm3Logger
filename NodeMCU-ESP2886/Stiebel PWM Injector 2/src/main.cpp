#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ModbusClientTCPasync.h>
#include <ArduinoOTA.h>
#include <time.h>
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>


// === GLOBAL CONSTANTS & VARIABLES ===
WiFiClient wifiClient;
ModbusClientTCPasync* modbusClient = nullptr;
OneWire oneWire(PIN_FLOW_TEMP);
DallasTemperature sensors(&oneWire);
float lastFlowTemp = NAN;
unsigned long lastTempRead = 0;
unsigned long postRunStart = 0;
const unsigned long POST_RUN_DURATION_MS = POST_RUN_DURATION_MIN * 60000UL;
static State previousState = State::ERROR;
static State currentState = State::ERROR;
unsigned long lastWiFiAttempt = 0;
bool wifiConnected = false;
unsigned long lastNTPSync = 0;
ESP8266WebServer server(80);
String logBuffer = "";
bool otaActive = false;
WiFiServer telnetServer(23);
WiFiClient telnetClient;


// === FUNCTION PROTOTYPES ===
void logMessage(const String& message, const LogLevel level = LogLevel::LOG_NORMAL);
float readFlowTemp();
const char* outputStatusName(const State state);
const char* stateName(const State state);
State evaluateState(const uint16_t status);
void handleOutputState(const State newState, const uint16_t status);
void syncTimeWithNTP();
void tryConnectWiFi();
float readPWMIn();
// Setup the webserver: serves main page and AJAX log endpoint
void setupWebServer() {
  server.on("/", []() {
    String html = F(R"rawliteral(
      <!DOCTYPE html>
      <html lang='en'>
      <head>
        <meta charset='UTF-8'>
        <meta name='viewport' content='width=device-width,initial-scale=1'>
        <title>Stiebel PWM Injector Log</title>
        <style>
          body { font-family: monospace; background: #222; color: #eee; margin: 0; padding: 0; }
          #log { white-space: pre-wrap; background: #111; padding: 1em; margin: 0; font-size: 1em; }
          .header {
            background: #333;
            margin: 0;
            padding: 0.5em 1em;
            font-size: 1.2em;
            display: flex;
            justify-content: space-between;
            align-items: center;
          }
          .left { text-align: left; }
          .right { text-align: right; font-size: 1em; color: #aaa; }
        </style>
      </head>
      <body>
        <div class='header'>
          Stiebel PWM Injector Log <span style="float:right; color:#aaa; font-size:1em;">Sjeiz<sup>©</sup></span>
        </div>
        <div id='log'>Loading...</div>
        <script>
          let lastLog = '';
          function fetchLog() {
            fetch('/log').then(r => r.text()).then(txt => {
              if (txt !== lastLog) {
                document.getElementById('log').textContent = txt;
                lastLog = txt;
              }
            }).catch(() => {});
          }
          setInterval(fetchLog, 2000);
          fetchLog();
        </script>
      </body>
      </html>
    )rawliteral");
    server.send(200, "text/html", html);
  });
  server.on("/log", []() {
    // Show log with latest lines at the top
    String reversedLog;
    int len = logBuffer.length();
    int end = len;
    // Walk backwards, find each line, prepend to reversedLog
    while (end > 0) {
      int start = logBuffer.lastIndexOf('\n', end - 2);
      if (start < 0) start = 0;
      String line = logBuffer.substring(start, end);
      reversedLog += line;
      end = start;
      if (start == 0) break;
    }
    server.send(200, "text/plain", reversedLog);
  });
  server.begin();
}


// Globale variabele voor actuele ISG status uit Modbus
volatile uint16_t isgStatus = ISG_MODBUS_READ_ERROR;

// Modbus async handlers (losse functies, zoals in eModbus voorbeeld)
void handleModbusData(ModbusMessage response, uint32_t token) {
  // Alleen verwerken als response correct is (FC=4, lengte >= 5)
  if (response.getFunctionCode() == 4 && response.size() >= 5) {
    // Modbus TCP: [serverID][FC][bytecount][dataHi][dataLo]
    uint8_t dataHi = response[3];
    uint8_t dataLo = response[4];
    uint16_t regValue = (dataHi << 8) | dataLo;
    isgStatus = regValue;
    String msg = String("[ModbusAsync] Response: 0x") + String(regValue, HEX);
    logMessage(msg, LogLevel::LOG_DEBUG);
  } else {
    String msg = String("[ModbusAsync] Response: serverID=") + response.getServerID() + ", FC=" + response.getFunctionCode() + ", Token=" + token + ", length=" + response.size() + ": ";
    for (auto& byte : response) msg += String(byte, HEX) + " ";
    logMessage(msg, LogLevel::LOG_DEBUG);
    isgStatus = ISG_MODBUS_READ_ERROR;
  }
}

void handleModbusError(Error error, uint32_t token) {
  ModbusError me(error);
  logMessage(String("[ModbusAsync] Error: ") + (const char*)me + ", token: " + String(token), LogLevel::LOG_DEBUG);
}

float readPWMIn() { return 75.0; }
float readFlowTemp() {
  unsigned long now = millis();
  if (now - lastTempRead > TEMP_READ_INTERVAL_MS || isnan(lastFlowTemp)) {
    sensors.requestTemperatures();
    lastFlowTemp = sensors.getTempCByIndex(0);
    lastTempRead = now;
  }
  return lastFlowTemp;
}

const char* outputStatusName(const State state) {
  switch (state) {
    case State::DEFROST: return "BLOCKED";
    case State::POST_RUN: return "FORCED";
    default: return "NORMAL";
  }
}

void logMessage(const String& message, const LogLevel level) {
  LogLevel minLevel = LogLevel::LOG_NORMAL;
  if (DEBUG) minLevel = LogLevel::LOG_DEBUG;
  else if (VERBOSE) minLevel = LogLevel::LOG_VERBOSE;
  if (static_cast<int>(level) > static_cast<int>(minLevel)) return;
  time_t now = time(nullptr);
  char buf[20];
  if (now > 100000) {
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
  } else {
    strcpy(buf, "1970-01-01 00:00:00");
  }
  String logLine = "[" + String(buf) + "] ";
  if (level == LogLevel::LOG_VERBOSE) logLine += "[VERBOSE] ";
  else if (level == LogLevel::LOG_DEBUG) logLine += "[DEBUG] ";
  logLine += message + "\r\n";
  Serial.print(logLine);
    if (logBuffer.length() > 0 && !logBuffer.endsWith("\n") && !logBuffer.endsWith("\r\n")) {
    logBuffer += "\r\n";
  }
  logBuffer += logLine;
  if (logBuffer.length() > 8000) logBuffer = logBuffer.substring(logBuffer.length() - 8000); // Max 8kB
  if (telnetClient && telnetClient.connected()) {
    telnetClient.write((const uint8_t*)logLine.c_str(), logLine.length());
    telnetClient.flush();
  }
}

State evaluateState(const uint16_t status) {
  if (status == ISG_MODBUS_READ_ERROR) return State::ERROR;
  bool compressor = status & ISG_STATUS_COMPRESSOR;
  bool defrost    = status & ISG_STATUS_DEFROSTING;
  bool cooling    = status & ISG_STATUS_COOLING;
  bool hotwater   = status & ISG_STATUS_HOT_WATER;
  bool heating    = status & ISG_STATUS_HEATING;

  if (!compressor) return State::STANDBY;
  if (defrost)     return State::DEFROST;
  if (cooling)     return State::COOLING;
  if (hotwater)    return State::HOT_WATER;
  if (heating)     return State::HEATING;
  return State::STANDBY;
}

void handleOutputState(const State newState, const uint16_t status) {
  switch (newState) {
    case State::DEFROST:
      digitalWrite(PIN_PUMP_ON, LOW);
      digitalWrite(PIN_PUMP_BLOCKED, HIGH);
      analogWrite(PIN_PWM_OUT, 0);
      break;
    case State::POST_RUN:
      digitalWrite(PIN_PUMP_ON, HIGH);
      digitalWrite(PIN_PUMP_BLOCKED, LOW);
      analogWrite(PIN_PWM_OUT, (int)(PWM_OUT_DUTY_PERCENT * 1023 / 100));
      break;
    default:
      digitalWrite(PIN_PUMP_ON, LOW);
      digitalWrite(PIN_PUMP_BLOCKED, LOW);
      analogWrite(PIN_PWM_OUT, 0);
      break;
  }
}

void syncTimeWithNTP() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  logMessage("Synchronizing time with NTP...");
  time_t now = time(nullptr);
  int retries = 10;
  while (now < 8 * 3600 * 2 && retries > 0) {
    delay(500);
    now = time(nullptr);
    retries--;
  }
  if (now > 8 * 3600 * 2) {
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    String msg = "\xE2\x9C\x85 NTP time: ";
    msg += buf;
    logMessage(msg);
  } else {
    logMessage("\xE2\x9D\x8C NTP sync failed!");
  }
}

void tryConnectWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME); // Set DHCP hostname for DNS
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  Serial.print(" ");
  int wifiTimeout = WIFI_TIMEOUT_SEC * 2; // WIFI_TIMEOUT_SEC x 2 x 500ms = seconds
  while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0) {
    delay(500);
    Serial.print(".");
    wifiTimeout--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    String msg = "\xE2\x9C\x85 Connection established! IP address: ";
    msg += WiFi.localIP().toString();
    logMessage(msg);
    wifiConnected = true;
    syncTimeWithNTP();
  } else {
    logMessage("\xE2\x9D\x8C WiFi connection failed!");
    wifiConnected = false;
  }
  lastWiFiAttempt = millis();
}

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_PUMP_ON, OUTPUT);
  pinMode(PIN_PUMP_BLOCKED, OUTPUT);
  // Activeer interne pull-up op OneWire pin (indien ondersteund)
  pinMode(PIN_FLOW_TEMP, INPUT_PULLUP);
  sensors.begin();
  // Initialize PWM output
  pinMode(PIN_PWM_OUT, OUTPUT);
  analogWriteFreq(PWM_OUT_FREQUENCY_HZ);
  int pwmValue = (int)(PWM_OUT_DUTY_PERCENT * 1023 / 100);
  analogWrite(PIN_PWM_OUT, pwmValue);
  tryConnectWiFi();
  lastNTPSync = millis();
  setupWebServer();
  logMessage("Setup complete", LogLevel::LOG_NORMAL);
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  // Modbus async initialisatie (ESP8266)
  // Initialiseer pas na WiFi, want IP/poort zijn dan bekend
  if (wifiConnected) {
    IPAddress isg_ip;
    WiFi.hostByName(ISG_HOST, isg_ip);
    modbusClient = new ModbusClientTCPasync(isg_ip, ISG_MODBUS_PORT);
    modbusClient->onDataHandler(&handleModbusData);
    modbusClient->onErrorHandler(&handleModbusError);
    modbusClient->setTimeout(10000); // 10s timeout
    modbusClient->setIdleTimeout(60000); // 60s idle timeout
    currentState = State::STANDBY;
  } else {
    currentState = State::ERROR;
  }
  // OTA setup
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    otaActive = true;
    digitalWrite(LED_BUILTIN, LOW); // LED aan
    logMessage("OTA Update Start", LogLevel::LOG_NORMAL);
  });
  ArduinoOTA.onEnd([]() {
    otaActive = false;
    digitalWrite(LED_BUILTIN, HIGH); // LED uit
    logMessage("OTA Update End", LogLevel::LOG_NORMAL);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    otaActive = false;
    digitalWrite(LED_BUILTIN, HIGH); // LED uit
    String errMsg = "OTA Error[" + String(error) + "]: ";
    if (error == OTA_AUTH_ERROR) errMsg += "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errMsg += "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errMsg += "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errMsg += "Receive Failed";
    else if (error == OTA_END_ERROR) errMsg += "End Failed";
    logMessage(errMsg, LogLevel::LOG_NORMAL);
  });
  ArduinoOTA.begin();

  // Initialize currentState after WiFi/Modbus setup
  bool modbusOk = wifiConnected; // Simuleer: Modbus OK als WiFi OK
  if (wifiConnected && modbusOk) {
    currentState = State::STANDBY;
    // OTA setup
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([]() {
      otaActive = true;
      digitalWrite(LED_BUILTIN, LOW); // LED aan
      logMessage("OTA Update Start", LogLevel::LOG_NORMAL);
    });
    ArduinoOTA.onEnd([]() {
      otaActive = false;
      digitalWrite(LED_BUILTIN, HIGH); // LED uit
      logMessage("OTA Update End", LogLevel::LOG_NORMAL);
    });
    ArduinoOTA.onError([](ota_error_t error) {
      otaActive = false;
      digitalWrite(LED_BUILTIN, HIGH); // LED uit
      String errMsg = "OTA Error[" + String(error) + "]: ";
      if (error == OTA_AUTH_ERROR) errMsg += "Auth Failed";
      else if (error == OTA_BEGIN_ERROR) errMsg += "Begin Failed";
      else if (error == OTA_CONNECT_ERROR) errMsg += "Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) errMsg += "Receive Failed";
      else if (error == OTA_END_ERROR) errMsg += "End Failed";
      logMessage(errMsg, LogLevel::LOG_NORMAL);
    });
    ArduinoOTA.begin();
  } else {
    currentState = State::ERROR;
  }
}

void loop() {
  // Telnet client connect/disconnect handling
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      telnetClient = telnetServer.accept();
      telnetClient.println("Stiebel PumpController Telnet Log");
      // Stuur direct de hele logbuffer naar nieuwe client
      telnetClient.write((const uint8_t*)logBuffer.c_str(), logBuffer.length());
      telnetClient.flush();
    } else {
      // Only one client at a time
      WiFiClient newClient = telnetServer.accept();
      newClient.println("Only one Telnet client supported.");
      newClient.stop();
    }
  }
  if (telnetClient && !telnetClient.connected()) {
    telnetClient.stop();
  }
  // 0. State tracking: Store current state before updating
  previousState = currentState;

  // Handle OTA updates
  ArduinoOTA.handle();

  // LED status: flash if not OTA, else fast blink (non-blocking, onafhankelijk van delay)
  static unsigned long lastLedToggle = 0;
  static bool ledState = false;
  unsigned long now = millis();
  unsigned long ledInterval = otaActive ? 100 : 800; // 800ms langzaam, 100ms snel
  if (now - lastLedToggle > ledInterval) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
    lastLedToggle = now;
  }

  // 1. Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      logMessage("\xE2\x9D\x8C WiFi connection lost!");
      wifiConnected = false;
      lastWiFiAttempt = millis();
    }
    if (millis() - lastWiFiAttempt >= (unsigned long)WIFI_RETRY_SEC * 1000UL) {
      tryConnectWiFi();
    }
  } else {
    if (!wifiConnected) {
      String msg = "\xE2\x9C\x85 Connection re-established! IP address: ";
      msg += WiFi.localIP().toString();
      logMessage(msg);
      wifiConnected = true;
    }
  }

  // 2. Resync NTP every hour if WiFi is connected
  if (wifiConnected && millis() - lastNTPSync >= NTP_RESYNC_INTERVAL_MS) {
    syncTimeWithNTP();
    lastNTPSync = millis();
  }

  // 3. Read inputs
  // a. ISG_OPERATING_STATUS uitlezen via Modbus async
  static unsigned long lastModbusMillis = 0;
  if (wifiConnected && modbusClient) {
    if (millis() - lastModbusMillis > 5000) {
      lastModbusMillis = millis();
      Error err = modbusClient->addRequest((uint32_t)lastModbusMillis, ISG_SLAVE_ID, READ_INPUT_REGISTER, ISG_OPERATING_STATUS_ADDR, 1);
      if (err != SUCCESS) {
        ModbusError me(err);
        logMessage(String("[ModbusAsync] Request error: ") + (const char*)me, LogLevel::LOG_DEBUG);
      }
    }
  }
  // isgStatus wordt nu asynchroon gevuld door handleModbusData()
  // b. Read PWM-in via GPIO (dummy function)
  const float pwmInVal = readPWMIn();
  // c. Read FlowTemp via GPIO
  const float flowTempVal = readFlowTemp();

  // 4. Determine state and transitions, including POST_RUN
  static unsigned long postRunStart = 0;

  State newState = evaluateState(isgStatus);

  // POST_RUN: HOT_WATER -> STANDBY
  if (currentState == State::HOT_WATER && newState == State::STANDBY) {
    currentState = State::POST_RUN;
    postRunStart = millis();
    logMessage("State: POST_RUN (forced)");
  } else if (currentState == State::POST_RUN) {
    // End POST_RUN after duration
    if (millis() - postRunStart > POST_RUN_DURATION_MS) {
      currentState = State::STANDBY;
      logMessage("State: POST_RUN ended");
    }
    // Cancel POST_RUN als ISG niet meer STANDBY is
    else if (newState != State::STANDBY) {
      currentState = newState;
      logMessage("State: POST_RUN cancelled");
    }
    // else: blijf in POST_RUN
  } else {
    currentState = newState;
  }

  // 5. Set outputs
  handleOutputState(currentState, isgStatus);

  // 6. Log status (with current values)
  // PWM-out is only active during POST_RUN
  static unsigned long lastLogTime = 0;
  String pwmOutVal = (currentState == State::POST_RUN) ? String(PWM_OUT_DUTY_PERCENT) + "%" : "OFF";
  if (now - lastLogTime >= ISG_POLL_INTERVAL_SEC * 1000UL) {
    char buf[200];
    String flowStr = (flowTempVal <= -126.9 && flowTempVal >= -127.1) ? "ERROR" : String(flowTempVal, 1) + "°C";
    String modbusStr;
    if (isgStatus == ISG_MODBUS_READ_ERROR) {
      modbusStr = "FAIL";
    } else {
      char hexbuf[12];
      snprintf(hexbuf, sizeof(hexbuf), "0x%04X", isgStatus);
      modbusStr = hexbuf;
    }
    snprintf(buf, sizeof(buf), "State:%s  Output:%s  PWM-in:%.1f%%  PWM-out:%s  Flow:%s  WiFi:%s  Modbus:%s",
      stateName(currentState),
      outputStatusName(currentState),
      pwmInVal,
      pwmOutVal.c_str(),
      flowStr.c_str(),
      wifiConnected ? "OK" : "FAIL",
      modbusStr.c_str()
    );
    logMessage(buf);
    lastLogTime = now;
  }
  server.handleClient();
}

// End of main.cpp