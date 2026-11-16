// Host stand-in for the parts of the Arduino/ESP8266 core that the tested sources use.
// Only for the native test environment; never part of the firmware.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdarg>
#include <ctime>
#include <string>
#include <functional>

using std::isnan;

// --- time: a clock the test sets -------------------------------------------------------------
extern uint32_t g_fakeMillis;
inline uint32_t millis() { return g_fakeMillis; }
inline void delay(unsigned long ms) { g_fakeMillis += ms; }
inline void yield() {}

// --- pins: every write is recorded so the test can read the outputs back ----------------------
#define HIGH 0x1
#define LOW  0x0
#define INPUT 0x00
#define OUTPUT 0x01
#define INPUT_PULLUP 0x02

static const uint8_t D2 = 4;
static const uint8_t D5 = 14;
static const uint8_t D6 = 12;
static const uint8_t D7 = 13;
static const uint8_t D8 = 15;
static const uint8_t LED_BUILTIN = 2;

extern int g_pinLevel[17];   // last digitalWrite per GPIO
extern int g_pinAnalog[17];  // last analogWrite per GPIO
extern int g_analogFreq;

inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t pin, uint8_t val) { if (pin < 17) g_pinLevel[pin] = val; }
inline int digitalRead(uint8_t pin) { return pin < 17 ? g_pinLevel[pin] : LOW; }
inline void analogWrite(uint8_t pin, int val) { if (pin < 17) g_pinAnalog[pin] = val; }
inline void analogWriteFreq(uint32_t freq) { g_analogFreq = (int)freq; }

// --- String: the subset the tested sources use ------------------------------------------------
class String {
public:
    String() {}
    String(const char* s) : s_(s ? s : "") {}
    String(const std::string& s) : s_(s) {}
    String(char c) : s_(1, c) {}
    String(int v) : s_(std::to_string(v)) {}
    String(unsigned int v) : s_(std::to_string(v)) {}
    String(long v) : s_(std::to_string(v)) {}
    String(unsigned long v) : s_(std::to_string(v)) {}
    String(float v, unsigned int decimals = 2) { char b[32]; snprintf(b, sizeof(b), "%.*f", decimals, v); s_ = b; }

    const char* c_str() const { return s_.c_str(); }
    unsigned int length() const { return (unsigned int)s_.size(); }

    String& operator+=(const String& o) { s_ += o.s_; return *this; }
    String& operator+=(const char* o) { s_ += (o ? o : ""); return *this; }
    String& operator+=(char c) { s_ += c; return *this; }

    friend String operator+(const String& a, const String& b) { return String(a.s_ + b.s_); }
    friend String operator+(const String& a, const char* b) { return String(a.s_ + (b ? b : "")); }
    friend String operator+(const char* a, const String& b) { return String(std::string(a ? a : "") + b.s_); }
    friend String operator+(const String& a, char c) { return String(a.s_ + c); }
    friend String operator+(const String& a, int v) { return String(a.s_ + std::to_string(v)); }

    bool operator==(const char* o) const { return s_ == (o ? o : ""); }

private:
    std::string s_;
};

#define F(x) (x)

// --- Serial: printf goes nowhere unless the test wants it --------------------------------------
struct FakeSerial {
    int printf(const char*, ...) { return 0; }
    void println(const char*) {}
    void begin(unsigned long) {}
};
extern FakeSerial Serial;
