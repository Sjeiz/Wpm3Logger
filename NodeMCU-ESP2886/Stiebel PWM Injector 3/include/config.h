


#ifndef CONFIG_H
#define CONFIG_H


// Interval voor temperatuur uitlezen (ms)
extern const unsigned long TEMP_READ_INTERVAL_MS;
// Interval voor NTP resync (ms)
extern const unsigned long NTP_RESYNC_INTERVAL_MS;

// Sentinel value for Modbus read failure
#define ISG_MODBUS_READ_ERROR 0xFFFF 

// Modbus slave ID
#define ISG_SLAVE_ID 1

// State for controlling hardware outputs
// State enum verwijderd, nu alleen State class uit State.h gebruiken

// Log level for serial output
enum class LogLevel {
  LOG_NORMAL,
  LOG_VERBOSE,
  LOG_DEBUG
};

// Post-run timer
extern const int POST_RUN_DURATION_MIN;
// Logging configuration
extern const bool DEBUG;
extern const bool VERBOSE;


// GPIO pin definitions
#define PIN_FLOW_TEMP    D2 // Input with pullup
#define PIN_PWM_OUT      D5 // Output PWM signal
#define PIN_PWM_IN       D6 // Input, no pullup
#define PIN_PUMP_ON      D7 // Output, normal low
#define PIN_PUMP_BLOCKED D8 // Output, normal low

// PWM configuratie
extern const int PWM_OUT_FREQUENCY_HZ;   // PWM frequentie (Hz)
extern const int PWM_OUT_DUTY_PERCENT;   // PWM duty cycle (%)

// ISG_OPERATING_STATUS bitflag definitions
#define ISG_STATUS_HK1_PUMP           0x0001 // B0
#define ISG_STATUS_HK2_PUMP           0x0002 // B1
#define ISG_STATUS_HEAT_UP_PROGRAM    0x0004 // B2
#define ISG_STATUS_NHZ_STAGES_RUNNING 0x0008 // B3
#define ISG_STATUS_HEATING            0x0010 // B4
#define ISG_STATUS_HOT_WATER          0x0020 // B5
#define ISG_STATUS_COMPRESSOR         0x0040 // B6
#define ISG_STATUS_SUMMER_MODE_ACTIVE 0x0080 // B7
#define ISG_STATUS_COOLING            0x0100 // B8
#define ISG_STATUS_DEFROSTING         0x0200 // B9
#define ISG_STATUS_SILENT_MODE_1      0x0400 // B10
#define ISG_STATUS_SILENT_MODE_2      0x0800 // B11

// Modbus/ISG configuration
extern const char* ISG_HOST;
extern const int ISG_MODBUS_PORT;
extern const int ISG_OPERATING_STATUS_ADDR;
extern const int ISG_POLL_INTERVAL_SEC;

// WiFi configuration
extern const char* HOSTNAME;
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const int WIFI_TIMEOUT_SEC;
extern const int WIFI_RETRY_SEC;

// NTP configuration
// OTA password configuration
extern const char* OTA_PASSWORD;

// DNS suffix configuration
extern const char* DNS_SUFFIX;
extern const char* NTP_SERVER;
extern const long GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;

#endif // CONFIG_H
