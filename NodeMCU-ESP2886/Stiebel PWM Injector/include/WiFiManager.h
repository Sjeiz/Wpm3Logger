#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include "Config.h"

// Initialize WiFi connection
void initWiFi(const char* ssid, const char* password);

// Check and maintain WiFi connection
// Reconnect if connection is lost
void updateWiFi();

// Get WiFi connection status
bool isWiFiConnected();

#endif
