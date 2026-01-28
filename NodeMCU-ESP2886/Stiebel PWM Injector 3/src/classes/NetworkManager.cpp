#include <ESP8266WiFi.h>
#include "NetworkManager.h"

NetworkManager::NetworkManager()
    : wifiConnected(false), lastWiFiAttempt(0), lastNTPSync(0), telnetServer(23) {}

void NetworkManager::logToTelnet(const String& msg) {
    if (telnetClient && telnetClient.connected()) {
        telnetClient.write((const uint8_t*)msg.c_str(), msg.length());
        telnetClient.flush();
    }
}

bool NetworkManager::isOtaActive() const {
    return otaActive;
}

void NetworkManager::begin() {
    tryConnectWiFi();
    setupOTA();
    setupTelnet();
}

void NetworkManager::loop() {
    ArduinoOTA.handle();
    handleTelnet();
    // NTP resync op basis van interval in minuten
    const unsigned long ntpResyncIntervalMs = NTP_RESYNC_INTERVAL_MIN * 60000UL;
    if (wifiConnected && millis() - lastNTPSync >= ntpResyncIntervalMs) {
        syncTimeWithNTP();
    }
}

bool NetworkManager::isWiFiConnected() const {
    return wifiConnected;
}

IPAddress NetworkManager::getLocalIP() const {
    return WiFi.localIP();
}

void NetworkManager::tryConnectWiFi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.hostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int wifiTimeout = WIFI_TIMEOUT_SEC * 2;
    while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0) {
        delay(500);
        wifiTimeout--;
    }
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    if (wifiConnected) {
        syncTimeWithNTP();
    }
    lastWiFiAttempt = millis();
}

void NetworkManager::syncTimeWithNTP() {
    // Alleen NTP-tijd synchroniseren (timezone is al gezet in setup)
    configTime(0, 0, NTP_SERVER);
    lastNTPSync = millis();
}

void NetworkManager::setupOTA() {
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([this]() {
        otaActive = true;
        Serial.println("OTA update started");
    });
    ArduinoOTA.onEnd([this]() {
        otaActive = false;
        Serial.println("OTA update finished");
    });
    ArduinoOTA.onError([this](ota_error_t error) {
        otaActive = false;
        Serial.printf("OTA Error[%u]\n", error);
    });
    ArduinoOTA.begin();
}

void NetworkManager::setupTelnet() {
    telnetServer.begin();
    telnetServer.setNoDelay(true);
}

void NetworkManager::handleTelnet() {
    if (telnetServer.hasClient()) {
        if (!telnetClient || !telnetClient.connected()) {
            telnetClient = telnetServer.accept();
        } else {
            WiFiClient newClient = telnetServer.accept();
            newClient.stop();
        }
    }
    if (telnetClient && telnetClient.connected() && telnetClient.available()) {
        // Input received, but do not echo
        telnetClient.readStringUntil('\n');
    }
}

IPAddress NetworkManager::resolveHostName(const char* host) const {
    IPAddress ip;
    if (WiFi.hostByName(host, ip)) {
        return ip;
    } else {
        return IPAddress(0, 0, 0, 0);
    }
}
