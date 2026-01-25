#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ModbusIP_ESP8266.h>
#include <time.h>
#include "config.h"

// Modbus client
ModbusIP mb;

// Timing
unsigned long lastReadTime = 0;

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

void initTime() {
  Serial.print("Synchroniseren met NTP server...");
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
  
  if (now > 24 * 3600) {
    timeInitialized = true;
    Serial.println(" OK!");
    
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    Serial.print("Huidige tijd: ");
    Serial.println(asctime(&timeinfo));
  } else {
    Serial.println(" TIMEOUT!");
    Serial.println("Tijd niet gesynchroniseerd, gebruik uptime");
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

void updateStateMachine(WpmState newState) {
  if (newState != stateMachine.currentState) {
    unsigned long timeInPreviousState = (millis() - stateMachine.stateEnteredAt) / 1000;
    
    // Enkele log regel met timestamp
    Serial.print("[");
    Serial.print(getTimestamp());
    Serial.print("] === STATE: ");
    Serial.print(stateToString(stateMachine.currentState));
    Serial.print(" → ");
    Serial.print(stateToString(newState));
    Serial.print(" (was ");
    Serial.print(timeInPreviousState);
    Serial.println(" sec in vorige state) ===");
    
    stateMachine.previousState = stateMachine.currentState;
    stateMachine.currentState = newState;
    stateMachine.stateEnteredAt = millis();
  }
}

void connectWiFi() {
  Serial.println();
  Serial.print("Verbinden met WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi verbonden!");
  Serial.print("IP adres: ");
  Serial.println(WiFi.localIP());
}

void connectModbus() {
  Serial.print("Verbinden met ISG-web op ");
  Serial.print(ISG_HOST);
  Serial.println("...");
  
  IPAddress isgIP;
  if (WiFi.hostByName(ISG_HOST, isgIP)) {
    Serial.print("ISG-web IP: ");
    Serial.println(isgIP);
    
    mb.client();
    if (mb.connect(isgIP, ISG_PORT)) {
      Serial.println("Modbus TCP verbonden!");
    } else {
      Serial.println("Modbus verbinding mislukt!");
    }
  } else {
    Serial.println("DNS lookup mislukt!");
  }
}

void readModbusRegister2500() {
  // Controleer verbinding
  if (!mb.isConnected(IPAddress())) {
    Serial.println("Modbus niet verbonden, opnieuw verbinden...");
    connectModbus();
    return;
  }
  
  // Lees status register 2500
  uint16_t res = mb.readHreg(IPAddress(), 2500, nullptr, 1, nullptr, ISG_SLAVE_ID);
  
  if (res == 0) {
    delay(100);
    uint16_t statusBits = mb.Hreg(2500);
    
    // Bepaal en update state
    WpmState newState = determineState(statusBits);
    updateStateMachine(newState);
    
    bool compressorRunning = (statusBits & (1 << 6)) != 0;
    unsigned long timeInState = (millis() - stateMachine.stateEnteredAt) / 1000;
    
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║     WPM3 Status                    ║");
    Serial.println("╚════════════════════════════════════╝");
    
    Serial.print("State:      ");
    Serial.println(stateToString(stateMachine.currentState));
    Serial.print("Compressor: ");
  
  initTime();
  delay(1000);
  
    Serial.println(compressorRunning ? "ACTIEF ●" : "UIT ○");
    Serial.print("Tijd:       ");
    Serial.print(timeInState);
    Serial.println(" sec in huidige state");
    
    Serial.print("\nBits:       0b");
    Serial.print(statusBits, BIN);
    Serial.print(" (0x");
    Serial.print(statusBits, HEX);
    Serial.println(")");
    
    // Toon alleen actieve bits
    Serial.println("\nActieve bits:");
    if (statusBits & (1 << 0)) Serial.println("  B0  HK1 Pomp");
    if (statusBits & (1 << 1)) Serial.println("  B1  HK2 Pomp");
    if (statusBits & (1 << 2)) Serial.println("  B2  Opwarmprogramma");
    if (statusBits & (1 << 3)) Serial.println("  B3  NHZ (bijverwarming)");
    if (statusBits & (1 << 4)) Serial.println("  B4  Verwarmingsmodus");
    if (statusBits & (1 << 5)) Serial.println("  B5  Warmwatermodus");
    if (statusBits & (1 << 6)) Serial.println("  B6  Compressor");
    if (statusBits & (1 << 7)) Serial.println("  B7  Zomerbedrijf");
    if (statusBits & (1 << 8)) Serial.println("  B8  Koeling");
    if (statusBits & (1 << 9)) Serial.println("  B9  Ontdooien");
    if (statusBits & (1 << 10)) Serial.println("  B10 Silent mode 1");
    if (statusBits & (1 << 11)) Serial.println("  B11 Silent mode 2");
    
    Serial.println("════════════════════════════════════\n");
  } else {
    Serial.println("❌ FOUT: Kan register 2500 niet lezen");
  }
}╔════════════════════════════════════╗");
  Serial.println("║   WPM3 Modbus Register 2500       ║");
  Serial.println("║   Status Reader                    ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  connectWiFi();
  delay(1000);
  connectModbus();
  
  Serial.println("\n✓ Setup voltooid!");
  Serial.println("Eerste uitlezing over 5 seconden...\n");
  lastReadTime = millis() - READ_INTERVAL + 5000;
}

void loop() {
  mb.task();
  
  // Periodiek uitlezen
  if (millis() - lastReadTime >= READ_INTERVAL) {
    readModbusRegister2500();
    lastReadTime = millis();
  }
  
  // Controleer WiFi verbinding
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ 
    lastReadTime = millis();
  }
  
  // Controleer WiFi verbinding
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi verbinding verloren, opnieuw verbinden...");
    connectWiFi();
    connectModbus();
  }
  
  delay(10);
}