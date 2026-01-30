#include <ESP8266WiFi.h>
#include "NetworkManager.h"
#include "helpers.h"
#include "globals.h"

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

    // Controleer WiFi en reconnect indien nodig
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        tryConnectWiFi();
    }

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
    logMessage("\xE2\x8C\x9B Initializing WiFi...");
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
        logMessage("\xE2\x9C\x85 WiFi connected: " + WiFi.localIP().toString());
        syncTimeWithNTP();
        // Herinitialiseer ModbusManager direct na WiFi reconnect
        extern IPAddress isgIp;
        modbusManager.begin(isgIp, ISG_PORT);
        // Dummy-read om eerste mislukte Modbus-read te negeren
        modbusManager.getByName("OPERATING_STATUS");
        if (modbusManager.isInitialized()) {
            logMessage("[INFO] ModbusManager initialized (WiFi reconnected)");
        } else {
            logMessage("[WARN] ModbusManager init failed (host unresolved)");
        }
    } else {
        logMessage("\xE2\x9D\x8C WiFi connection failed!");
    }
    lastWiFiAttempt = millis();
}

void NetworkManager::syncTimeWithNTP() {
    // Alleen NTP-tijd synchroniseren (timezone is al gezet in setup)
    logMessage("\xE2\x8C\x9B Synchronizing time with NTP...");
    configTime(0, 0, NTP_SERVER);
    delay(1000); // Geef tijd voor NTP sync
    time_t now = time(nullptr);
    if (now > 24 * 3600) {
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        String msg = String("\xE2\x9C\x85 NTP time: ") + buf;
        logMessage(msg);
    } else {
        logMessage("\xE2\x9D\x8C NTP sync failed!");
    }
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
