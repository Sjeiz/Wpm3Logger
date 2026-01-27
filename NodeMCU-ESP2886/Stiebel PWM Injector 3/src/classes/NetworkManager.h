#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <ESP8266WebServer.h>
#include "config.h"

class NetworkManager {
public:
    NetworkManager();
    void begin();
    void loop();
    bool isWiFiConnected() const;
    IPAddress getLocalIP() const;
    void syncTimeWithNTP();
    void setupTelnet();
    void handleTelnet();
    void logToTelnet(const String& msg);
    bool isOtaActive() const;
    bool wifiConnected;
    unsigned long lastWiFiAttempt;
    unsigned long lastNTPSync;
    WiFiServer telnetServer;
    WiFiClient telnetClient;
    void tryConnectWiFi();
    void setupOTA();
    bool otaActive = false;
};

#endif // NETWORKMANAGER_H
