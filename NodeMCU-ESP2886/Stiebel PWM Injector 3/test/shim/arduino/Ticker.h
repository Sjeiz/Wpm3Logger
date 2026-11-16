// Host stand-in for the ESP8266 Ticker: the LED blink timer does nothing on the host.
// Only for the native test environment; never part of the firmware.
#pragma once

#include <functional>

class Ticker {
public:
    template <typename F>
    void attach_ms(uint32_t, F) {}
    void detach() {}
};
