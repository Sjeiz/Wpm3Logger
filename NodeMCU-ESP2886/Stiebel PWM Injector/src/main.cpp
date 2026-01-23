// pwm D6 input tester

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const int PIN_PWM_OUT = D1;          // PWM bron
const int PIN_PWM_IN  = D6;          // optocoupler transistorzijde
const int LED_PIN     = LED_BUILTIN; // onboard LED (active LOW)

const char* WIFI_SSID = "SjeizWifi_IoT";
const char* WIFI_PW   = "VerbindenMetSjeizW1f1_IoT";

ESP8266WebServer server(80);

float dutyIn = 0.0;
float freqIn = 0.0;
bool pwmDetected = false;

unsigned long lastBlink = 0;
bool ledState = false;

// -----------------------------
// Webpagina
// -----------------------------
void handleRoot() {
  String html =
    "<html><head><meta http-equiv='refresh' content='1' /></head><body>"
    "<h2>PWM Monitor (D6)</h2>"
    "<p><b>Duty:</b> " + String(dutyIn, 1) + "%<br>"
    "<b>Freq:</b> " + String(freqIn, 1) + " Hz<br>"
    "<b>Status:</b> " + String(pwmDetected ? "PWM gedetecteerd" : "GEEN PWM") +
    "</p></body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}

// -----------------------------
// WiFi connectie
// -----------------------------
void connectWiFiOnce() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PW);

  Serial.print("WiFi verbinden");
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi timeout, geen verbinding.");
  }
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_PWM_OUT, OUTPUT);      // PWM bron
  pinMode(PIN_PWM_IN, INPUT_PULLUP); // optocoupler open collector
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);        // LED aan = script draait

  // PWM op D1
  analogWriteRange(1023);
  analogWriteFreq(1000);             // 1 kHz
  analogWrite(PIN_PWM_OUT, 512);     // ~50% duty

  connectWiFiOnce();

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Webserver gestart op poort 80");
}

// -----------------------------
// Loop
// -----------------------------
void loop() {
  server.handleClient();

  // PWM meting
  unsigned long highTime = pulseIn(PIN_PWM_IN, HIGH, 20000);
  unsigned long lowTime  = pulseIn(PIN_PWM_IN, LOW,  20000);

  pwmDetected = (highTime > 0 && lowTime > 0);

  if (pwmDetected) {
    float period = (highTime + lowTime) / 1000000.0;
    freqIn = 1.0 / period;
    dutyIn = (highTime * 100.0) / (highTime + lowTime);
  } else {
    freqIn = 0.0;
    dutyIn = 0.0;
  }

  // LED gedrag
  unsigned long now = millis();

  if (pwmDetected) {
    // Langzaam knipperen (1 Hz)
    if (now - lastBlink >= 500) {
      lastBlink = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW); // active LOW
    }
  } else {
    // Snel flashen (5 Hz)
    if (now - lastBlink >= 100) {
      lastBlink = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
  }

  // Logging
  static unsigned long lastLog = 0;
  if (now - lastLog >= 1000) {
    lastLog = now;
    Serial.print("PWM: ");
    Serial.print(pwmDetected ? "JA" : "NEE");
    Serial.print("  high=");
    Serial.print(highTime);
    Serial.print(" us  low=");
    Serial.print(lowTime);
    Serial.print(" us  duty=");
    Serial.print(dutyIn, 1);
    Serial.print("%  freq=");
    Serial.print(freqIn, 1);
    Serial.println(" Hz");
  }
}