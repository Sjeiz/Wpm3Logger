// WebLogger refresh interval (seconds)
extern const int WEBLOGGER_REFRESH_INTERVAL_MIN;
#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>

// WebLogger buffer size (in bytes)
extern const int WEBLOGGER_BUFFER_SIZE;
// WebLogger temperature margin for logging
extern const float WEBLOGGER_TEMP_MARGE;
// WebLogger minimum detailregel interval (minuten)
extern const int WEBLOGGER_DETAIL_INTERVAL_MIN;


// Interval voor temperatuur uitlezen (ms)
// Interval voor NTP resync (min)
extern const int NTP_RESYNC_INTERVAL_MIN;


// Modbus register addresses en namen (voor gebruik in ModbusManager)



// Log level for serial output
enum class LogLevel {LOG_NORMAL, LOG_VERBOSE, LOG_DEBUG};

// Post-run timer
extern const int POST_RUN_DURATION_MIN;
// Logging configuration
extern const bool DEBUG;
extern const bool VERBOSE;


// GPIO pin definitions
#define PIN_FLOW_TEMP    D2 // Input with pullup
#define PIN_PWM_OUT      D5 // Output PWM signal
#define PIN_PWM_IN       D6 // Input, no pullup
#define PIN_PUMP_FORCE   D7 // Output, normal low
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
extern const int ISG_PORT;
extern const int ISG_OPERATING_STATUS_ADDR;
extern const int ISG_POLL_INTERVAL_SEC;
#define ISG_MODBUS_READ_ERROR 0xFFFF 
#define ISG_SLAVE_ID 1
struct ModbusRegDef {const char* name; uint16_t address;};
struct ModbusConfig {const ModbusRegDef* regs; uint8_t count;};
extern const ModbusRegDef MODBUS_REGS[];
extern const ModbusConfig MODBUS_CONFIG;


// WiFi configuration
extern const char* HOSTNAME;
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const int WIFI_TIMEOUT_SEC;
extern const int WIFI_RETRY_SEC;


// NTP & tijdzone configuratie
extern const char* NTP_SERVER;
extern const char* TIMEZONE; // bv. "CET-1CEST,M3.5.0/2,M10.5.0/3" voor Europa/Amsterdam

// OTA password configuration
extern const char* OTA_PASSWORD;

// DNS suffix configuration
extern const char* DNS_SUFFIX;


// State pointers (voor centralTransition)
class State;
extern State* errorState;
extern State* defrostState;
extern State* coolingState;
extern State* heatingState;
extern State* hotWaterState;
extern State* standbyState;
extern State* postRunState;

#endif // CONFIG_H
