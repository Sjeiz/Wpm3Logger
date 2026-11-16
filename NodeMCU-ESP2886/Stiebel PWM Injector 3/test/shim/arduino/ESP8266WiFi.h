// Host stand-in for ESP8266WiFi: IPAddress and a WiFiClient whose server side the test scripts.
// Only for the native test environment; never part of the firmware.
#pragma once

#include "Arduino.h"

class IPAddress {
public:
    IPAddress() : b_{0, 0, 0, 0} {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : b_{a, b, c, d} {}
    bool isSet() const { return b_[0] || b_[1] || b_[2] || b_[3]; }
    String toString() const {
        char s[16];
        snprintf(s, sizeof(s), "%u.%u.%u.%u", b_[0], b_[1], b_[2], b_[3]);
        return String(s);
    }
    bool operator==(const IPAddress& o) const { return memcmp(b_, o.b_, 4) == 0; }
private:
    uint8_t b_[4];
};

// What the fake ISG does with each request. The response bytes become readable respondDelayMs
// after the connect (fake clock); with splitAt > 0 only the first splitAt bytes arrive then and the
// rest splitDelayMs later. After the last byte the server closes the connection, as the ISG does on
// HTTP/1.0.
struct FakeIsgServer {
    bool acceptConnect = true;
    std::string response;
    uint32_t respondDelayMs = 2500;   // measured: ~2.5 s per call
    size_t splitAt = 0;
    uint32_t splitDelayMs = 0;
    bool neverRespond = false;      // keeps the connection open without sending anything
    std::string lastRequest;
    int connectCount = 0;
};
extern FakeIsgServer g_fakeIsg;

class WiFiClient {
public:
    void setTimeout(unsigned long) {}
    void setNoDelay(bool) {}

    int connect(const IPAddress&, uint16_t) {
        g_fakeIsg.connectCount++;
        g_fakeIsg.lastRequest.clear();
        pos_ = 0;
        connectAt_ = millis();
        open_ = g_fakeIsg.acceptConnect;
        return open_ ? 1 : 0;
    }

    size_t print(const String& s) {
        if (open_) g_fakeIsg.lastRequest += s.c_str();
        return s.length();
    }

    int available() {
        if (!open_) return 0;
        return (int)(delivered() - pos_);
    }

    int read() {
        if (available() <= 0) return -1;
        return (uint8_t)g_fakeIsg.response[pos_++];
    }

    // Like the ESP8266 core: still "connected" while unread data is buffered.
    uint8_t connected() {
        if (!open_) return 0;
        if (available() > 0) return 1;
        if (g_fakeIsg.neverRespond) return 1;
        return delivered() >= g_fakeIsg.response.size() && millis() >= respondAt() ? 0 : 1;
    }

    void stop() { open_ = false; }

private:
    size_t delivered() const {
        if (g_fakeIsg.neverRespond || millis() < respondAt()) return 0;
        size_t total = g_fakeIsg.response.size();
        if (g_fakeIsg.splitAt > 0 && g_fakeIsg.splitAt < total &&
            millis() < respondAt() + g_fakeIsg.splitDelayMs) {
            return g_fakeIsg.splitAt;
        }
        return total;
    }

    uint32_t respondAt() const { return connectAt_ + g_fakeIsg.respondDelayMs; }

    size_t pos_ = 0;
    uint32_t connectAt_ = 0;
    bool open_ = false;
};
