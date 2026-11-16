#include "IsgApiManager.h"
#include <ArduinoJson.h>
#include <math.h>
#include "../config.h"
#include "../helpers.h"

IsgApiManager::IsgApiManager(const IsgConfig& cfg)
    : cfg(cfg),
      port(80),
      values(nullptr),
      responseLen(0),
      overflow(false),
      busy(false),
      busySince(0),
      lastPoll(0),
      initialized(false)
{
    values = new float[cfg.count];
    invalidateAll();
    response[0] = '\0';
}

IsgApiManager::~IsgApiManager() {
    client.stop();
    if (values) delete[] values;
}

void IsgApiManager::begin(IPAddress newIp, uint16_t newPort) {
    // A (re)start drops any request in flight; the next poll starts a fresh one.
    client.stop();
    busy = false;
    if (!newIp.isSet()) {
        initialized = false;
        logMessage(String("[ERROR] Invalid IP address for IsgApiManager: ") + newIp.toString(), LogLevel::LOG_DEBUG);
        return;
    }
    ip = newIp;
    port = newPort;
    logMessage("[DEBUG] IsgApiManager using http://" + ip.toString() + ":" + String(port) + ISG_API_PATH, LogLevel::LOG_DEBUG);
    initialized = true;
}

void IsgApiManager::loop() {
    if (!initialized) return;

    if (busy) {
        collectResponse();
        return;
    }

    // A new poll ISG_POLL_INTERVAL_SEC after the previous one started.
    if (millis() - lastPoll < ISG_POLL_INTERVAL_SEC * 1000UL) return;
    startRequest();
}

bool IsgApiManager::isBusy() const {
    return busy;
}

bool IsgApiManager::isInitialized() const {
    return initialized;
}

float IsgApiManager::getValue(uint16_t webid) const {
    for (uint8_t i = 0; i < cfg.count; i++) {
        if (cfg.regs[i].webid == webid) return values[i];
    }
    return NAN;
}

uint16_t IsgApiManager::getWord(uint16_t webid) const {
    float v = getValue(webid);
    // ISG_READ_ERROR (0xFFFF) is reserved as the "no value" marker, so it is not a valid word.
    if (isnan(v) || v < 0.0f || v >= (float)ISG_READ_ERROR || v != floorf(v)) {
        return ISG_READ_ERROR;
    }
    return (uint16_t)v;
}

uint16_t IsgApiManager::getHundredths(uint16_t webid) const {
    float v = getValue(webid);
    if (isnan(v) || v < 0.0f) return ISG_READ_ERROR;
    float hundredths = roundf(v * 100.0f);
    if (hundredths >= (float)ISG_READ_ERROR) return ISG_READ_ERROR;
    return (uint16_t)hundredths;
}

void IsgApiManager::startRequest() {
    lastPoll = millis();
    responseLen = 0;
    overflow = false;
    response[0] = '\0';

    // connect() blocks until connected or this timeout; on the LAN it took 10 ms (measured).
    client.setTimeout(1000);
    if (!client.connect(ip, port)) {
        finish("connect failed");
        return;
    }
    client.setNoDelay(true);

    // HTTP/1.0: the ISG's lighttpd then answers with Content-Length and closes the connection,
    // instead of HTTP/1.1 chunked transfer encoding (measured 16-11-2026).
    String request = String("GET ") + ISG_API_PATH + "?ids=";
    for (uint8_t i = 0; i < cfg.count; i++) {
        if (i > 0) request += ',';
        request += String(cfg.regs[i].webid);
    }
    request += " HTTP/1.0\r\nHost: ";
    request += ISG_HOST;
    request += "\r\nConnection: close\r\n\r\n";
    client.print(request);

    busy = true;
    busySince = millis();
}

void IsgApiManager::collectResponse() {
    // Non-blocking: take only what has already arrived.
    while (client.available() > 0) {
        int c = client.read();
        if (c < 0) break;
        if (responseLen < RESPONSE_BUFFER_SIZE - 1) {
            response[responseLen++] = (char)c;
        } else {
            overflow = true;
        }
    }
    response[responseLen] = '\0';

    // The server closes the connection after the body (HTTP/1.0).
    if (!client.connected() && client.available() == 0) {
        const char* error = nullptr;
        if (overflow) {
            finish("response larger than buffer");
        } else if (parseResponse(error)) {
            finish(nullptr);
        } else {
            finish(error);
        }
        return;
    }

    if (millis() - busySince > (uint32_t)ISG_HTTP_TIMEOUT_MS) {
        finish("timeout");
    }
}

bool IsgApiManager::parseResponse(const char*& error) {
    // Status line: "HTTP/1.x 200 ..."
    if (strncmp(response, "HTTP/1.", 7) != 0 || responseLen < 12) {
        error = "no HTTP status line";
        return false;
    }
    int status = atoi(response + 9);
    if (status != 200) {
        error = "HTTP status not 200";
        return false;
    }
    const char* body = strstr(response, "\r\n\r\n");
    if (!body) {
        error = "no response body";
        return false;
    }
    body += 4;

    JsonDocument doc;
    DeserializationError jsonError = deserializeJson(doc, body);
    if (jsonError || !doc.is<JsonObject>()) {
        error = "body is not a JSON object";
        return false;
    }

    // A web ID absent from the payload has no measurement: NAN, as HA's unknown.
    char key[8];
    for (uint8_t i = 0; i < cfg.count; i++) {
        snprintf(key, sizeof(key), "%u", (unsigned)cfg.regs[i].webid);
        JsonVariant v = doc[key];
        if (!v.isNull() && (v.is<float>() || v.is<long>())) {
            values[i] = v.as<float>();
        } else {
            values[i] = NAN;
        }
    }
    return true;
}

void IsgApiManager::finish(const char* error) {
    client.stop();
    busy = false;
    if (error) {
        invalidateAll();
        logMessage(String("[DEBUG] IsgApiManager request failed: ") + error, LogLevel::LOG_DEBUG);
    }
}

void IsgApiManager::invalidateAll() {
    for (uint8_t i = 0; i < cfg.count; i++) {
        values[i] = NAN;
    }
}
