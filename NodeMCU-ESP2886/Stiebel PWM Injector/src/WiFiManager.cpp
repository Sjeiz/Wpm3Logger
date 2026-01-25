#include "WiFiManager.h"
#include <ESP8266WiFi.h>

// WiFi state
static const char* wifiSSID = nullptr;
static const char* wifiPassword = nullptr;
static unsigned long lastConnectionAttempt = 0;
static bool wasConnected = false;

// Initialize WiFi connection
void initWiFi(const char* ssid, const char* password) {
  wifiSSID = ssid;
  wifiPassword = password;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPassword);

  Serial.print("WiFi verbinden");
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECTION_TIMEOUT) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP: ");
    Serial.println(WiFi.localIP());
    wasConnected = true;
  } else {
    Serial.println("WiFi timeout, geen verbinding.");
    wasConnected = false;
  }
  
  lastConnectionAttempt = millis();
}

// Check and maintain WiFi connection
void updateWiFi() {
  // If currently connected, just check status
  if (WiFi.status() == WL_CONNECTED) {
    if (!wasConnected) {
      Serial.print("WiFi hersteld, IP: ");
      Serial.println(WiFi.localIP());
      wasConnected = true;
    }
    return;
  }
  
  // Connection lost, try to reconnect periodically
  if (!wasConnected) {
    // Already marked as disconnected
    if (millis() - lastConnectionAttempt >= WIFI_RECONNECT_INTERVAL) {
      Serial.println("WiFi verbinding verbroken, poging tot herstel...");
      WiFi.reconnect();
      lastConnectionAttempt = millis();
    }
  } else {
    // Just lost connection
    Serial.println("WiFi verbinding verbroken!");
    wasConnected = false;
    lastConnectionAttempt = millis();
  }
}

// Get WiFi connection status
bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}
