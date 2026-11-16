// WebLogger refresh interval (seconds)

#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>

// WebLogger
extern const float WEBLOGGER_TEMP_DELTA;
extern const float WEBLOGGER_FLOW_DELTA;
extern const float WEBLOGGER_FLOW_DELTA_PCT;
extern const float WEBLOGGER_PWMIN_DELTA_PCT;
extern const int WEBLOGGER_REFRESH_PAGE_SEC;
extern const int WEBLOGGER_DETAIL_INTERVAL_SEC;


// Interval for temperature reading (ms)
// Interval for NTP resync (min)
extern const int NTP_RESYNC_INTERVAL_MIN;


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

// PWM configuration
extern const int PWM_OUT_FREQUENCY_HZ;   // PWM frequency (Hz)
extern const int PWM_OUT_DUTY_PERCENT;   // PWM duty cycle (%)

// --- ISG web IDs -------------------------------------------------------------------------------
// Same web IDs, bit numbers and names as Home Assistant: the register is sensor.isg_<webid>, a bit is
// binary_sensor.isg_<webid>_<bit>. Names are Stiebel's (English UI name, German original in
// brackets), as in kennis/webid_catalogus.csv of the stiebel-isg repo.

// Web ID 471 = OPERATING STATUS (BETRIEBSSTATUS), bit field, menu DIAGNOSE > ANLAGE > BETRIEBSSTATUS.
// HA: sensor.isg_471 ("ISG Operating Status").
#define ISG_WEBID_OPERATING_STATUS 471
#define ISG_471_HEATING_CIRCUIT_1_PUMP            0x0001 // bit 0  HEATING CIRCUIT 1 PUMP (HK 1 PUMPE): switches with the compressor only; not the HKP terminal (measured)
#define ISG_471_HEATING_CIRCUIT_2_PUMP            0x0002 // bit 1  HEATING CIRCUIT 2 PUMP (HK 2 PUMPE): same as 402 bit 5 / 485 bit 1, the empty MKP terminal; the floor pump is on HKP (402 bit 4 / 485 bit 0)
#define ISG_471_HEAT_UP_PROGRAM                   0x0004 // bit 2  HEAT-UP PROGRAM (AUFHEIZPROGRAMM): screed heat-up program active
#define ISG_471_ELECTRIC_EMERGENCY_BOOSTER_HEATER 0x0008 // bit 3  ELECTRIC EMERGENCY/BOOSTER HEATER (NHZ STUFEN): electric backup heater stages on
#define ISG_471_HEATING                           0x0010 // bit 4  HEATING (HEIZBETRIEB): heat pump runs for space heating
#define ISG_471_DHW_MODE                          0x0020 // bit 5  DHW MODE (WARMWASSERBETRIEB): heat pump runs for domestic hot water
#define ISG_471_COMPRESSOR                        0x0040 // bit 6  COMPRESSOR (VERDICHTER): compressor runs
#define ISG_471_SUMMER_MODE                       0x0080 // bit 7  SUMMER MODE (SOMMERBETRIEB): summer mode, no space heating
#define ISG_471_COOLING_MODE                      0x0100 // bit 8  COOLING MODE (KUEHLBETRIEB): heat pump runs for cooling
#define ISG_471_DEFROST                           0x0200 // bit 9  DEFROST (ABTAUBETRIEB): outdoor unit is defrosting
#define ISG_471_SILENT_MODE_1                     0x0400 // bit 10 SILENT MODE 1 (SILENTMODE 1)
#define ISG_471_SILENT_MODE_2                     0x0800 // bit 11 SILENT MODE 2 (SILENTMODE 2)

// Web ID 438 = FLOW RATE (VOLUMENSTROM), l/min, menu INFO > ANLAGE > HEIZUNG: heating water flow.
// HA: sensor.isg_438 ("ISG Flow Rate").
#define ISG_WEBID_FLOW_RATE 438


// ISG HTTP API configuration (endpoint isg_api.php, read path: GET, no token needed)
extern const char* ISG_HOST;
extern const int ISG_HTTP_PORT;
extern const char* ISG_API_PATH;
extern const int ISG_HTTP_TIMEOUT_MS;
extern const int ISG_POLL_INTERVAL_SEC;
extern const int ISG_POLL_STABLETIME_SEC;
#define ISG_READ_ERROR 0xFFFF // "no value": request failed or the web ID was absent from the payload
struct IsgRegDef {const char* name; uint16_t webid;};
struct IsgConfig {const IsgRegDef* regs; uint8_t count;};
extern const IsgRegDef ISG_REGS[];
extern const IsgConfig ISG_CONFIG;


// WiFi configuration
extern const char* HOSTNAME;
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const int WIFI_TIMEOUT_SEC;
extern const int WIFI_RETRY_SEC;


// NTP & timezone configuration
extern const char* NTP_SERVER;
extern const char* TIMEZONE; // e.g. "CET-1CEST,M3.5.0/2,M10.5.0/3" for Europe/Amsterdam

// OTA password configuration
extern const char* OTA_PASSWORD;

// DNS suffix configuration
extern const char* DNS_SUFFIX;


// State pointers (for centralTransition)
class State;
extern State* errorState;
extern State* defrostState;
extern State* coolingState;
extern State* heatingState;
extern State* hotWaterState;
extern State* standbyState;
extern State* postRunState;

#endif // CONFIG_H
