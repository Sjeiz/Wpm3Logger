#ifndef CONFIG_H
#define CONFIG_H

// Alleen user-configuratie, geen types/structs!

// WiFi configuratie
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// ISG-web Modbus TCP configuratie
extern const char* ISG_HOST;
extern const uint16_t ISG_PORT;
extern const uint8_t ISG_SLAVE_ID;

// Timing configuratie
extern const unsigned long READ_INTERVAL;

// PWM configuratie (voor D5 post-run timer)
extern const uint16_t PWM_FREQUENCY;
extern const uint8_t PWM_DUTY_CYCLE;

// Debug configuratie
extern const bool DEBUG_ENABLED;

// GPIO pin-definities
constexpr uint8_t PIN_FLOWTEMP_IN = D2;      // GPIO4  - Flow temperature input (with pull-up)
constexpr uint8_t PIN_PWM_OUT = D5;          // GPIO14 - PWM output
constexpr uint8_t PIN_PWM_IN = D6;           // GPIO12 - PWM input (no pull-up)
constexpr uint8_t PIN_PUMP_FORCED = D7;      // GPIO13 - Forces pump HK2 ON during post-run timer
constexpr uint8_t PIN_PUMP_BLOCKED = D8;     // GPIO15 - Blocks pump HK2 during defrost

// Log buffer grootte
constexpr int LOG_BUFFER_SIZE = 60;

// NTP instellingen
extern const char* NTP_SERVER;
extern const long GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;

#endif // CONFIG_H
