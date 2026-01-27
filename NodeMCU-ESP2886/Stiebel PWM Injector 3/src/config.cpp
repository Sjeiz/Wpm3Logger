// Always include headers first!
#include "config.h"

// Interval voor temperatuur uitlezen (ms)
const unsigned long TEMP_READ_INTERVAL_MS = 5000;
// Interval voor NTP resync (ms)
const unsigned long NTP_RESYNC_INTERVAL_MS = 3600000UL;




// PWM configuratie
const int PWM_OUT_FREQUENCY_HZ = 150;
const int PWM_OUT_DUTY_PERCENT = 30;

// Post-run timer
const int POST_RUN_DURATION_MIN = 3;

// Logging configuration
const bool DEBUG = false;
const bool VERBOSE = false;

// Modbus/ISG configuration
const char* ISG_HOST = "servicewelt.iot.cheizoo.lan";
const int ISG_MODBUS_PORT = 502;
const int ISG_OPERATING_STATUS_ADDR = 2500;
const int ISG_POLL_INTERVAL_SEC = 10;

// NTP configuration
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;

// WiFi credentials
const char* HOSTNAME = "stiebelpumpcontrol";
const char* WIFI_SSID = "SjeizWifi_IoT";
const char* WIFI_PASSWORD = "VerbindenMetSjeizW1f1_IoT";
const int WIFI_TIMEOUT_SEC = 30;
const int WIFI_RETRY_SEC = 60;

// OTA password configuration
const char* OTA_PASSWORD = "StiebelPumpControl";
