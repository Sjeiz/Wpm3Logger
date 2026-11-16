// Host tests for the ISG read path and the state machine (PlatformIO env "native").
//
// Runs the firmware's own IsgApiManager, StateManager/centralTransition and OutputManager against
// ISG API responses. The network is a scripted fake (test/shim/arduino/ESP8266WiFi.h): each poll
// gets the scripted response ~2.5 s after the connect, as the real ISG answers.
//
// Responses:
//  - live:      captured from the ISG (fixtures/live_*.http), compared with what Home Assistant
//               showed at the same moment;
//  - synthetic: the live heating capture with only the 471 value (or one field) changed, so every
//               state, transition and output of the state machine is exercised.
//
// Web ID 471 = OPERATING STATUS (BETRIEBSSTATUS); bit names and meanings are the ISG_471_* in
// src/config.h. Web ID 438 = FLOW RATE (VOLUMENSTROM), l/min. Web ID 402 = PUMP (PUMPENSTATUS) and
// web ID 485 = STATUS (pump and mixer outputs), both bit fields, only read here to compare with HA.

#include <unity.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "config.h"
#include "IsgApiManager.h"
#include "StateManager.h"
#include "OutputManager.h"

// ---- globals the shims and the linked sources expect ------------------------------------------
uint32_t g_fakeMillis = 0;
int g_pinLevel[17] = {0};
int g_pinAnalog[17] = {0};
int g_analogFreq = 0;
FakeSerial Serial;
FakeIsgServer g_fakeIsg;
unsigned long stateEnterTime = 0;          // defined in globals.cpp on the device
extern unsigned long postRunEndTime;       // StateManager.cpp

static std::vector<std::string> g_log;
void logMessage(const String& message, const LogLevel) { g_log.push_back(message.c_str()); }
void logMessage(const String& message) { g_log.push_back(message.c_str()); }

// One StateManager for the whole run: its destructor deletes the shared state objects.
static StateManager sm;

// ---- fixtures ---------------------------------------------------------------------------------
static std::string fixturePath(const char* name) {
    std::string file = __FILE__;
    size_t slash = file.find_last_of("/\\");
    return file.substr(0, slash + 1) + "fixtures/" + name;
}

static std::string loadFixture(const char* name) {
    std::ifstream in(fixturePath(name), std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    std::string data = ss.str();
    if (data.empty()) {
        std::string msg = std::string("fixture missing or empty: ") + fixturePath(name);
        TEST_FAIL_MESSAGE(msg.c_str());
    }
    return data;
}

// Live capture 16-11-2026 23:09:22 (22:09:22Z), ids=471,438,402,485.
static const char* LIVE_HEATING = "live_20261116T220922Z_heating.http";

// Replaces the body and corrects Content-Length.
static std::string withBody(const std::string& http, const std::string& body) {
    size_t sep = http.find("\r\n\r\n");
    std::string head = http.substr(0, sep);
    size_t cl = head.find("Content-Length: ");
    if (cl != std::string::npos) {
        size_t end = head.find("\r\n", cl);
        head.replace(cl, end - cl, "Content-Length: " + std::to_string(body.size()));
    }
    return head + "\r\n\r\n" + body;
}

static std::string bodyOf(const std::string& http) {
    return http.substr(http.find("\r\n\r\n") + 4);
}

// Replaces the JSON value of one web ID in the body, as raw JSON text.
static std::string withField(const std::string& http, const char* webid, const std::string& jsonValue) {
    std::string body = bodyOf(http);
    std::string key = std::string("\"") + webid + "\":";
    size_t at = body.find(key);
    if (at == std::string::npos) TEST_FAIL_MESSAGE("web ID not in base response");
    size_t start = at + key.size();
    size_t end = body.find_first_of(",}", start);
    body.replace(start, end - start, jsonValue);
    return withBody(http, body);
}

// The live heating capture with web ID 471 set to another status word.
static std::string withStatus(uint16_t word) {
    return withField(loadFixture(LIVE_HEATING), "471", std::to_string(word));
}

static std::string withStatusLine(const std::string& http, const std::string& statusLine) {
    size_t eol = http.find("\r\n");
    return statusLine + http.substr(eol);
}

// ---- the controller loop, in the order of main.cpp loop() -------------------------------------
static const uint32_t TICK_MS = 100;
// Longest wait for the next answer: one poll interval plus the ~2.5 s the ISG takes, with margin.
static const uint32_t NEXT_ANSWER_MS = ISG_POLL_INTERVAL_SEC * 1000UL + 3000;
static const uint32_t POSTRUN_PWM_RAW = (uint32_t)((100 - PWM_OUT_DUTY_PERCENT) * 1023 / 100);

struct Controller {
    IsgApiManager isg{ISG_CONFIG};
    OutputManager out;

    void start() {
        isg.begin(IPAddress(192, 168, 12, 10), ISG_HTTP_PORT);
        out.begin();
        sm.begin(errorState);                       // start state after boot
        out.loop(sm.currentStateName());
    }
    void tick() {
        if (isg.isInitialized()) isg.loop();
        uint16_t status = isg.isInitialized() ? isg.getWord(ISG_WEBID_OPERATING_STATUS) : 0;
        sm.update(status);
        out.loop(sm.currentStateName());
    }
    void runFor(uint32_t ms) {
        for (uint32_t t = 0; t < ms; t += TICK_MS) { g_fakeMillis += TICK_MS; tick(); }
    }
    // Ticks until the state is reached; returns the fake time at entry.
    uint32_t runUntil(const char* state, uint32_t maxMs) {
        for (uint32_t t = 0; t <= maxMs; t += TICK_MS) {
            if (strcmp(sm.currentStateName(), state) == 0) return g_fakeMillis;
            g_fakeMillis += TICK_MS;
            tick();
        }
        std::string msg = std::string("state not reached: ") + state + ", still " + sm.currentStateName();
        TEST_FAIL_MESSAGE(msg.c_str());
        return 0;
    }
};

static void serve(const std::string& response) {
    g_fakeIsg.response = response;
    g_fakeIsg.neverRespond = false;
    g_fakeIsg.acceptConnect = true;
}

// Feeds a status word straight into the state machine and the outputs, without the ISG round trip,
// for rules that act inside one poll interval.
static void feed(Controller& c, uint16_t word, uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += TICK_MS) {
        g_fakeMillis += TICK_MS;
        sm.update(word);
        c.out.loop(sm.currentStateName());
    }
}

static void assertState(const char* expected) {
    TEST_ASSERT_EQUAL_STRING(expected, sm.currentStateName());
}

// Output per state, statemachine.md "Outputs per state".
static void assertPassiveOutputs() {
    TEST_ASSERT_EQUAL_MESSAGE(LOW, g_pinLevel[PIN_PUMP_FORCE], "D7 PUMP_FORCE");
    TEST_ASSERT_EQUAL_MESSAGE(LOW, g_pinLevel[PIN_PUMP_BLOCKED], "D8 PUMP_BLOCKED");
    TEST_ASSERT_EQUAL_MESSAGE(0, g_pinAnalog[PIN_PWM_OUT], "D5 PWM-out");
}
static void assertCoolingOutputs() {
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, g_pinLevel[PIN_PUMP_FORCE], "D7 PUMP_FORCE");
    TEST_ASSERT_EQUAL_MESSAGE(LOW, g_pinLevel[PIN_PUMP_BLOCKED], "D8 PUMP_BLOCKED");
    TEST_ASSERT_EQUAL_MESSAGE(0, g_pinAnalog[PIN_PWM_OUT], "D5 PWM-out");
}
static void assertPostRunOutputs() {
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, g_pinLevel[PIN_PUMP_FORCE], "D7 PUMP_FORCE");
    TEST_ASSERT_EQUAL_MESSAGE(LOW, g_pinLevel[PIN_PUMP_BLOCKED], "D8 PUMP_BLOCKED");
    TEST_ASSERT_EQUAL_MESSAGE(POSTRUN_PWM_RAW, g_pinAnalog[PIN_PWM_OUT], "D5 PWM-out");
}

// Status words: the live word 83 = 0x0053 = bits 0,1,4,6 (HK1 pump, HK2 pump, HEATING, COMPRESSOR).
static const uint16_t W_LIVE_HEATING = 83;
static const uint16_t W_BASE = W_LIVE_HEATING & ~(ISG_471_HEATING | ISG_471_COMPRESSOR); // pumps only
static const uint16_t W_STANDBY = W_BASE;                                                 // compressor off
static const uint16_t W_HEATING = W_LIVE_HEATING;
static const uint16_t W_HOT_WATER = W_BASE | ISG_471_DHW_MODE | ISG_471_COMPRESSOR;
static const uint16_t W_COOLING = W_BASE | ISG_471_COOLING_MODE | ISG_471_COMPRESSOR;
static const uint16_t W_DEFROST = W_LIVE_HEATING | ISG_471_DEFROST;
static const uint16_t W_SUMMER_OFF = W_BASE | ISG_471_SUMMER_MODE;                      // compressor off, summer
static const uint16_t W_COMPRESSOR_ONLY = W_BASE | ISG_471_COMPRESSOR;

void setUp() {
    g_fakeMillis = 1000000;
    memset(g_pinLevel, 0, sizeof(g_pinLevel));
    memset(g_pinAnalog, 0xFF, sizeof(g_pinAnalog));   // "never written"
    g_fakeIsg = FakeIsgServer();
    g_log.clear();
}
void tearDown() {}

// Brings a fresh controller into a state: start, serve the word, wait for the state, then let the
// minimum state time pass so the next change is not held.
static void settleIn(Controller& c, uint16_t word, const char* state) {
    c.start();
    serve(withStatus(word));
    c.runUntil(state, NEXT_ANSWER_MS);
    c.runFor(ISG_POLL_STABLETIME_SEC * 1000UL + 1000);
    assertState(state);
}

// =============================================================================================
// L: live responses, compared with Home Assistant at the same moment
// =============================================================================================

// L1: the controller's request, and its parse of the live capture, against HA 23:09:22:
// sensor.isg_471 = 83, sensor.isg_438 = 14.29.
void test_L1_live_parse_matches_ha() {
    Controller c;
    c.start();
    serve(loadFixture(LIVE_HEATING));
    c.runFor(NEXT_ANSWER_MS);
    TEST_ASSERT_TRUE(g_fakeIsg.lastRequest.rfind("GET /isg_api.php?ids=471,438 HTTP/1.0\r\n", 0) == 0);
    TEST_ASSERT_TRUE(g_fakeIsg.lastRequest.find("Host: servicewelt.iot.cheizoo.lan\r\n") != std::string::npos);
    TEST_ASSERT_EQUAL_UINT16(83, c.isg.getWord(ISG_WEBID_OPERATING_STATUS));
    TEST_ASSERT_EQUAL_FLOAT(14.29f, c.isg.getValue(ISG_WEBID_FLOW_RATE));
    TEST_ASSERT_EQUAL_UINT16(1429, c.isg.getHundredths(ISG_WEBID_FLOW_RATE));
}

// L2: bit decoding against HA's own bit entities at 23:09:22. The parser reads 402 and 485 here
// only for this comparison; the controller does not request them.
void test_L2_live_bits_match_ha() {
    static const IsgRegDef regs[] = {{"OPERATING STATUS", 471}, {"FLOW RATE", 438}, {"PUMP", 402}, {"STATUS", 485}};
    static const IsgConfig cfg = {regs, 4};
    IsgApiManager isg(cfg);
    isg.begin(IPAddress(192, 168, 12, 10), ISG_HTTP_PORT);
    serve(loadFixture(LIVE_HEATING));
    for (int i = 0; i < 100; i++) { g_fakeMillis += TICK_MS; isg.loop(); }

    uint16_t w471 = isg.getWord(471), w402 = isg.getWord(402), w485 = isg.getWord(485);
    TEST_ASSERT_EQUAL_UINT16(83, w471);     // sensor.isg_471
    TEST_ASSERT_EQUAL_UINT16(561, w402);    // sensor.isg_402
    TEST_ASSERT_EQUAL_UINT16(2051, w485);   // sensor.isg_485
    // binary_sensor.isg_471_<bit>: 4 on, 5 off, 6 on, 7 off, 8 off, 9 off
    TEST_ASSERT_TRUE(w471 & ISG_471_HEATING);
    TEST_ASSERT_FALSE(w471 & ISG_471_DHW_MODE);
    TEST_ASSERT_TRUE(w471 & ISG_471_COMPRESSOR);
    TEST_ASSERT_FALSE(w471 & ISG_471_SUMMER_MODE);
    TEST_ASSERT_FALSE(w471 & ISG_471_COOLING_MODE);
    TEST_ASSERT_FALSE(w471 & ISG_471_DEFROST);
    // binary_sensor.isg_402_4 on (HTG CIRC PUMP 1), isg_402_5 on (HTG CIRC PUMP 2)
    TEST_ASSERT_TRUE(w402 & (1u << 4));
    TEST_ASSERT_TRUE(w402 & (1u << 5));
    // binary_sensor.isg_485_0 on (HEATING CIRCUIT PUMP), isg_485_1 on (MIXER PUMP), isg_485_13 off (COOLING MODE output)
    TEST_ASSERT_TRUE(w485 & (1u << 0));
    TEST_ASSERT_TRUE(w485 & (1u << 1));
    TEST_ASSERT_FALSE(w485 & (1u << 13));
}

// L3: the live capture drives the state machine to HEATING (HA: compressor on, heating on), passive outputs.
void test_L3_live_heating_state() {
    Controller c;
    c.start();
    serve(loadFixture(LIVE_HEATING));
    c.runUntil("HEATING", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// L4: the endpoint's real 400 answer (unknown web ID) is no value -> ERROR, passive.
void test_L4_live_http400_is_error() {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    serve(loadFixture("live_http400_unknown_webid.http"));
    c.runUntil("ERROR", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT16(ISG_READ_ERROR, c.isg.getWord(ISG_WEBID_OPERATING_STATUS));
    assertPassiveOutputs();
}

// L5: a real 200 answer without 471 (the endpoint omits a value that is no measurement) -> ERROR;
// the flow rate in that answer is still taken.
void test_L5_live_471_absent_is_error() {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    serve(loadFixture("live_471_absent.http"));
    c.runUntil("ERROR", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT16(1429, c.isg.getHundredths(ISG_WEBID_FLOW_RATE));
    assertPassiveOutputs();
}

// L6: a real empty object {} -> no values -> ERROR.
void test_L6_live_empty_object_is_error() {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    serve(loadFixture("live_empty_object.http"));
    c.runUntil("ERROR", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT16(ISG_READ_ERROR, c.isg.getHundredths(ISG_WEBID_FLOW_RATE));
    assertPassiveOutputs();
}

// =============================================================================================
// E: failures and invalid fields -> no value -> ERROR with passive outputs
// =============================================================================================

static void expectErrorFrom(const std::string& response) {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    serve(response);
    c.runUntil("ERROR", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT16(ISG_READ_ERROR, c.isg.getWord(ISG_WEBID_OPERATING_STATUS));
    assertPassiveOutputs();
}

// E1: timeout. Old values stay while the request is in flight (HEATING held), ERROR once
// ISG_HTTP_TIMEOUT_MS has passed.
void test_E1_timeout() {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    g_fakeIsg.neverRespond = true;
    // run until the next request has started
    int connects = g_fakeIsg.connectCount;
    while (g_fakeIsg.connectCount == connects) c.runFor(TICK_MS);
    uint32_t start = g_fakeMillis;
    c.runFor(ISG_HTTP_TIMEOUT_MS - 200);
    assertState("HEATING");
    uint32_t at = c.runUntil("ERROR", 1000);
    TEST_ASSERT_TRUE(at - start >= (uint32_t)ISG_HTTP_TIMEOUT_MS);
    assertPassiveOutputs();
}

// E2: connection refused.
void test_E2_connect_refused() {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    g_fakeIsg.acceptConnect = false;
    c.runUntil("ERROR", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// E3: the server closes without sending anything.
void test_E3_closed_without_data() { expectErrorFrom(""); }

// E4: HTTP 500 with the endpoint's own error text.
void test_E4_http500() {
    expectErrorFrom(withBody(withStatusLine(loadFixture(LIVE_HEATING), "HTTP/1.0 500 Internal Server Error"),
                             "{\"error\":\"isg_tester returned no complete register dump\"}"));
}

// E5: body is not JSON (cut off mid-object).
void test_E5_truncated_json() { expectErrorFrom(withBody(loadFixture(LIVE_HEATING), "{\"471\":8")); }

// E6: body is HTML, not a JSON object.
void test_E6_html_body() { expectErrorFrom(withBody(loadFixture(LIVE_HEATING), "<html>error</html>")); }

// E7: a JSON array instead of an object.
void test_E7_json_array() { expectErrorFrom(withBody(loadFixture(LIVE_HEATING), "[83,14.29]")); }

// E8: invalid 471 values: text, fraction, negative, the error marker, out of 16 bits, null, object.
void test_E8_471_text() { expectErrorFrom(withField(loadFixture(LIVE_HEATING), "471", "\"83\"")); }
void test_E8_471_fraction() { expectErrorFrom(withField(loadFixture(LIVE_HEATING), "471", "83.5")); }
void test_E8_471_negative() { expectErrorFrom(withField(loadFixture(LIVE_HEATING), "471", "-1")); }
void test_E8_471_65535() { expectErrorFrom(withField(loadFixture(LIVE_HEATING), "471", "65535")); }
void test_E8_471_too_big() { expectErrorFrom(withField(loadFixture(LIVE_HEATING), "471", "70000")); }
void test_E8_471_null() { expectErrorFrom(withField(loadFixture(LIVE_HEATING), "471", "null")); }
void test_E8_471_object() { expectErrorFrom(withField(loadFixture(LIVE_HEATING), "471", "{\"v\":83}")); }

// E9: an invalid 438 only loses the flow rate; the state machine keeps working on 471.
void test_E9_438_invalid_keeps_state() {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    serve(withField(loadFixture(LIVE_HEATING), "438", "\"fast\""));
    c.runFor(NEXT_ANSWER_MS);
    assertState("HEATING");
    TEST_ASSERT_EQUAL_UINT16(ISG_READ_ERROR, c.isg.getHundredths(ISG_WEBID_FLOW_RATE));
    TEST_ASSERT_EQUAL_UINT16(W_HEATING, c.isg.getWord(ISG_WEBID_OPERATING_STATUS));
}

// E10: a response larger than the receive buffer.
void test_E10_oversized_response() {
    std::string http = loadFixture(LIVE_HEATING);
    size_t eol = http.find("\r\n");
    http.insert(eol + 2, "X-Padding: " + std::string(600, 'x') + "\r\n");
    expectErrorFrom(http);
}

// E11: no blank line between headers and body.
void test_E11_no_header_end() { expectErrorFrom("HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n"); }

// E12: not HTTP at all.
void test_E12_not_http() { expectErrorFrom("garbage"); }

// E13: the answer arrives in two parts over several loop passes and is still read whole.
void test_E13_split_response() {
    Controller c;
    c.start();
    std::string http = withStatus(W_HEATING);
    serve(http);
    g_fakeIsg.splitAt = http.find("\r\n\r\n") + 2;
    g_fakeIsg.splitDelayMs = 300;
    c.runUntil("HEATING", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT16(1429, c.isg.getHundredths(ISG_WEBID_FLOW_RATE));
}

// E14: no valid ISG address: the client is not initialised and returns no value.
void test_E14_invalid_ip() {
    IsgApiManager isg(ISG_CONFIG);
    isg.begin(IPAddress(0, 0, 0, 0), ISG_HTTP_PORT);
    TEST_ASSERT_FALSE(isg.isInitialized());
    TEST_ASSERT_EQUAL_UINT16(ISG_READ_ERROR, isg.getWord(ISG_WEBID_OPERATING_STATUS));
}

// =============================================================================================
// S: every state, transition and output of the state machine (synthetic words from the live capture)
// =============================================================================================

// S1: start-up: ERROR with passive outputs until the first answer arrives.
void test_S1_startup() {
    Controller c;
    c.start();
    assertState("ERROR");
    assertPassiveOutputs();
    serve(withStatus(W_HEATING));
    c.runFor(2000);                 // first request in flight, no answer yet
    assertState("ERROR");
    assertPassiveOutputs();
}

// S2: ERROR -> HEATING on the first good answer, without waiting for the minimum state time.
void test_S2_error_recovery_immediate() {
    Controller c;
    c.start();
    serve(withStatus(W_HEATING));
    uint32_t at = c.runUntil("HEATING", NEXT_ANSWER_MS);
    TEST_ASSERT_TRUE(at - 1000000 < (uint32_t)ISG_POLL_STABLETIME_SEC * 1000UL);
    assertPassiveOutputs();
}

// S3: minimum state time: HEATING -> STANDBY is held until HEATING has lasted ISG_POLL_STABLETIME_SEC.
void test_S3_minimum_state_time() {
    Controller c;
    c.start();
    serve(withStatus(W_HEATING));
    uint32_t entered = c.runUntil("HEATING", NEXT_ANSWER_MS);
    serve(withStatus(W_STANDBY));
    c.runFor(ISG_POLL_STABLETIME_SEC * 1000UL - 200);
    assertState("HEATING");
    uint32_t left = c.runUntil("STANDBY", NEXT_ANSWER_MS);
    TEST_ASSERT_TRUE(left - entered >= (uint32_t)ISG_POLL_STABLETIME_SEC * 1000UL);
    assertPassiveOutputs();
}

// S4: HEATING -> STANDBY when the compressor stops (no post-run after heating).
void test_S4_heating_to_standby() {
    Controller c;
    settleIn(c, W_HEATING, "HEATING");
    serve(withStatus(W_STANDBY));
    c.runUntil("STANDBY", NEXT_ANSWER_MS);
    c.runFor(60000);
    assertState("STANDBY");
    assertPassiveOutputs();
}

// S5: DEFROST is immediate, also inside the minimum state time, and keeps the outputs passive
// (D8 not blocked). Fed straight into the state machine: a poll answer arrives at most once per
// interval, which is not shorter than the minimum state time. A normal change at the same moment
// is held (control).
void test_S5_defrost_immediate_passive() {
    Controller c;
    c.start();
    feed(c, W_HEATING, TICK_MS);
    assertState("HEATING");
    feed(c, W_HEATING, 5000);
    feed(c, W_STANDBY, TICK_MS);
    assertState("HEATING");                         // control: held by the minimum state time
    feed(c, W_DEFROST, TICK_MS);
    assertState("DEFROST");
    assertPassiveOutputs();

    // and over the ISG path
    Controller d;
    settleIn(d, W_HEATING, "HEATING");
    serve(withStatus(W_DEFROST));
    d.runUntil("DEFROST", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// S6: DEFROST -> HEATING after the defrost, once DEFROST has lasted the minimum state time.
void test_S6_defrost_back_to_heating() {
    Controller c;
    settleIn(c, W_DEFROST, "DEFROST");
    serve(withStatus(W_HEATING));
    c.runUntil("HEATING", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// S7: HOT_WATER -> POST_RUN outside summer mode: D7 forced, PWM injected; STANDBY after the timer.
void test_S7_hot_water_post_run() {
    Controller c;
    settleIn(c, W_HOT_WATER, "HOT_WATER");
    assertPassiveOutputs();
    serve(withStatus(W_STANDBY));
    uint32_t entered = c.runUntil("POST_RUN", NEXT_ANSWER_MS);
    assertPostRunOutputs();
    TEST_ASSERT_TRUE(postRunEndTime != 0);
    c.runFor(POST_RUN_DURATION_MIN * 60000UL - 1000);
    assertState("POST_RUN");
    assertPostRunOutputs();
    uint32_t left = c.runUntil("STANDBY", 5000);
    TEST_ASSERT_TRUE(left - entered >= (uint32_t)POST_RUN_DURATION_MIN * 60000UL);
    TEST_ASSERT_EQUAL_UINT32(0, postRunEndTime);
    assertPassiveOutputs();
}

// S8: HOT_WATER -> STANDBY in summer mode (471 bit 7): no post-run.
void test_S8_hot_water_summer_no_post_run() {
    Controller c;
    settleIn(c, W_HOT_WATER, "HOT_WATER");
    serve(withStatus(W_SUMMER_OFF));
    c.runUntil("STANDBY", NEXT_ANSWER_MS);
    c.runFor(60000);
    assertState("STANDBY");
    TEST_ASSERT_EQUAL_UINT32(0, postRunEndTime);
    assertPassiveOutputs();
}

// Brings a controller into POST_RUN with the minimum state time passed.
static void intoPostRun(Controller& c) {
    settleIn(c, W_HOT_WATER, "HOT_WATER");
    serve(withStatus(W_STANDBY));
    c.runUntil("POST_RUN", NEXT_ANSWER_MS);
    c.runFor(ISG_POLL_STABLETIME_SEC * 1000UL + 1000);
    assertState("POST_RUN");
}

// S9: POST_RUN cancelled by COOLING MODE (bit 8) -> COOLING, timer cleared, D7 forced, no PWM.
void test_S9_post_run_cancel_cooling() {
    Controller c;
    intoPostRun(c);
    serve(withStatus(W_COOLING));
    c.runUntil("COOLING", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT32(0, postRunEndTime);
    assertCoolingOutputs();
}

// S10: POST_RUN cancelled by HEATING (bit 4), compressor bit not required -> HEATING.
void test_S10_post_run_cancel_heating_without_compressor() {
    Controller c;
    intoPostRun(c);
    serve(withStatus(W_BASE | ISG_471_HEATING));
    c.runUntil("HEATING", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT32(0, postRunEndTime);
    assertPassiveOutputs();
}

// S11: POST_RUN cancelled by DHW MODE (bit 5) -> HOT_WATER.
void test_S11_post_run_cancel_hot_water() {
    Controller c;
    intoPostRun(c);
    serve(withStatus(W_HOT_WATER));
    c.runUntil("HOT_WATER", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT32(0, postRunEndTime);
    assertPassiveOutputs();
}

// S12: POST_RUN cancel priority: COOLING before HEATING before HOT_WATER.
void test_S12_post_run_cancel_priority() {
    Controller c;
    intoPostRun(c);
    serve(withStatus(W_BASE | ISG_471_COOLING_MODE | ISG_471_HEATING | ISG_471_DHW_MODE));
    c.runUntil("COOLING", NEXT_ANSWER_MS);

    Controller d;
    intoPostRun(d);
    serve(withStatus(W_BASE | ISG_471_HEATING | ISG_471_DHW_MODE));
    d.runUntil("HEATING", NEXT_ANSWER_MS);
}

// S13: POST_RUN -> DEFROST cancels the timer; after the defrost the state is STANDBY, not POST_RUN.
void test_S13_post_run_defrost_cancels() {
    Controller c;
    intoPostRun(c);
    serve(withStatus(W_BASE | ISG_471_DEFROST));
    c.runUntil("DEFROST", NEXT_ANSWER_MS);
    TEST_ASSERT_EQUAL_UINT32(0, postRunEndTime);
    assertPassiveOutputs();
    serve(withStatus(W_STANDBY));
    c.runUntil("STANDBY", ISG_POLL_STABLETIME_SEC * 1000UL + NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// S14: POST_RUN -> ERROR on a timeout cancels the timer and goes passive; recovery goes to STANDBY.
void test_S14_post_run_error_and_recovery() {
    Controller c;
    intoPostRun(c);
    g_fakeIsg.neverRespond = true;
    c.runUntil("ERROR", NEXT_ANSWER_MS + ISG_HTTP_TIMEOUT_MS);
    TEST_ASSERT_EQUAL_UINT32(0, postRunEndTime);
    assertPassiveOutputs();
    serve(withStatus(W_STANDBY));
    c.runUntil("STANDBY", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// S15: COOLING (compressor + bit 8): D7 forced, no PWM; leaving COOLING restores the passive outputs.
void test_S15_cooling_and_back() {
    Controller c;
    settleIn(c, W_STANDBY, "STANDBY");
    serve(withStatus(W_COOLING));
    c.runUntil("COOLING", NEXT_ANSWER_MS);
    assertCoolingOutputs();
    c.runFor(ISG_POLL_STABLETIME_SEC * 1000UL + 1000);
    serve(withStatus(W_STANDBY));
    c.runUntil("STANDBY", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// S16: compressor on: COOLING before HEATING before HOT_WATER; compressor alone -> STANDBY.
void test_S16_compressor_on_priority() {
    Controller a;
    settleIn(a, W_STANDBY, "STANDBY");
    serve(withStatus(W_BASE | ISG_471_COMPRESSOR | ISG_471_COOLING_MODE | ISG_471_HEATING | ISG_471_DHW_MODE));
    a.runUntil("COOLING", NEXT_ANSWER_MS);

    Controller b;
    settleIn(b, W_STANDBY, "STANDBY");
    serve(withStatus(W_BASE | ISG_471_COMPRESSOR | ISG_471_HEATING | ISG_471_DHW_MODE));
    b.runUntil("HEATING", NEXT_ANSWER_MS);

    Controller d;
    settleIn(d, W_STANDBY, "STANDBY");
    serve(withStatus(W_HOT_WATER));
    d.runUntil("HOT_WATER", NEXT_ANSWER_MS);

    Controller e;
    settleIn(e, W_HEATING, "HEATING");
    serve(withStatus(W_COMPRESSOR_ONLY));
    e.runUntil("STANDBY", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// S17: ERROR is immediate, also inside the minimum state time; COOLING -> ERROR releases D7.
// Fed straight into the state machine (see S5), then over the ISG path with a refused connection.
void test_S17_error_immediate_from_cooling() {
    Controller c;
    c.start();
    feed(c, W_COOLING, TICK_MS);
    assertState("COOLING");
    assertCoolingOutputs();
    feed(c, W_COOLING, 5000);
    feed(c, ISG_READ_ERROR, TICK_MS);
    assertState("ERROR");
    assertPassiveOutputs();

    Controller d;
    d.start();
    serve(withStatus(W_COOLING));
    d.runUntil("COOLING", NEXT_ANSWER_MS);
    assertCoolingOutputs();
    g_fakeIsg.acceptConnect = false;
    d.runUntil("ERROR", NEXT_ANSWER_MS);
    assertPassiveOutputs();
}

// S18: D8 PUMP_BLOCKED is never set, in any state.
void test_S18_never_blocked() {
    const uint16_t words[] = {W_HEATING, W_HOT_WATER, W_STANDBY, W_COOLING, W_DEFROST, W_SUMMER_OFF};
    Controller c;
    c.start();
    for (uint16_t w : words) {
        serve(withStatus(w));
        for (int i = 0; i < 600; i++) {
            c.runFor(TICK_MS);
            TEST_ASSERT_EQUAL(LOW, g_pinLevel[PIN_PUMP_BLOCKED]);
        }
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_L1_live_parse_matches_ha);
    RUN_TEST(test_L2_live_bits_match_ha);
    RUN_TEST(test_L3_live_heating_state);
    RUN_TEST(test_L4_live_http400_is_error);
    RUN_TEST(test_L5_live_471_absent_is_error);
    RUN_TEST(test_L6_live_empty_object_is_error);
    RUN_TEST(test_E1_timeout);
    RUN_TEST(test_E2_connect_refused);
    RUN_TEST(test_E3_closed_without_data);
    RUN_TEST(test_E4_http500);
    RUN_TEST(test_E5_truncated_json);
    RUN_TEST(test_E6_html_body);
    RUN_TEST(test_E7_json_array);
    RUN_TEST(test_E8_471_text);
    RUN_TEST(test_E8_471_fraction);
    RUN_TEST(test_E8_471_negative);
    RUN_TEST(test_E8_471_65535);
    RUN_TEST(test_E8_471_too_big);
    RUN_TEST(test_E8_471_null);
    RUN_TEST(test_E8_471_object);
    RUN_TEST(test_E9_438_invalid_keeps_state);
    RUN_TEST(test_E10_oversized_response);
    RUN_TEST(test_E11_no_header_end);
    RUN_TEST(test_E12_not_http);
    RUN_TEST(test_E13_split_response);
    RUN_TEST(test_E14_invalid_ip);
    RUN_TEST(test_S1_startup);
    RUN_TEST(test_S2_error_recovery_immediate);
    RUN_TEST(test_S3_minimum_state_time);
    RUN_TEST(test_S4_heating_to_standby);
    RUN_TEST(test_S5_defrost_immediate_passive);
    RUN_TEST(test_S6_defrost_back_to_heating);
    RUN_TEST(test_S7_hot_water_post_run);
    RUN_TEST(test_S8_hot_water_summer_no_post_run);
    RUN_TEST(test_S9_post_run_cancel_cooling);
    RUN_TEST(test_S10_post_run_cancel_heating_without_compressor);
    RUN_TEST(test_S11_post_run_cancel_hot_water);
    RUN_TEST(test_S12_post_run_cancel_priority);
    RUN_TEST(test_S13_post_run_defrost_cancels);
    RUN_TEST(test_S14_post_run_error_and_recovery);
    RUN_TEST(test_S15_cooling_and_back);
    RUN_TEST(test_S16_compressor_on_priority);
    RUN_TEST(test_S17_error_immediate_from_cooling);
    RUN_TEST(test_S18_never_blocked);
    return UNITY_END();
}
