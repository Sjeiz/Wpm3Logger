#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "config.h"

// Reads the ISG over its HTTP API (isg_api.php, stiebel-isg repo) -- the same endpoint and the same
// web IDs Home Assistant reads (sensor.isg_api -> sensor.isg_<webid>).
//
// One GET per poll asks for every web ID in ISG_CONFIG at once. The endpoint takes ~2.5 s to answer
// (it runs a full isg_tester dump per call), so the request is non-blocking: loop() sends it and then
// only collects whatever bytes have arrived, so the main loop (PWM-in sampling, web server, OTA,
// telnet) keeps running while the ISG works.
//
// A value is NAN when the request failed, timed out, or the endpoint left the web ID out of its
// payload (the endpoint omits a value that is not a measurement; HA shows that as unknown).
class IsgApiManager {
public:
    explicit IsgApiManager(const IsgConfig& cfg);
    ~IsgApiManager();

    void begin(IPAddress ip, uint16_t port = 80);
    void loop();

    bool isBusy() const;
    bool isInitialized() const;

    // Value as the API returns it (already scaled to human units), or NAN when there is none.
    float getValue(uint16_t webid) const;

    // A bit-field register as a 16-bit word, or ISG_READ_ERROR when there is no valid value.
    uint16_t getWord(uint16_t webid) const;

    // Value in hundredths of its unit (web ID 438 FLOW RATE: 0.01 l/min, the unit the logging works in),
    // or ISG_READ_ERROR when there is no valid value.
    uint16_t getHundredths(uint16_t webid) const;

private:
    void startRequest();
    void collectResponse();
    void finish(const char* error);
    bool parseResponse(const char*& error);
    void invalidateAll();

    static const size_t RESPONSE_BUFFER_SIZE = 512; // headers ~170 B + body ~25 B, measured 16-11-2026

    const IsgConfig& cfg;
    WiFiClient client;
    IPAddress ip;
    uint16_t port;

    float* values;
    char response[RESPONSE_BUFFER_SIZE];
    size_t responseLen;
    bool overflow;

    bool busy;
    uint32_t busySince;
    uint32_t lastPoll;
    bool initialized;
};
