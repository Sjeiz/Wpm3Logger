
#include "config.h"
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
const char* WIFI_SSID = "SjeizWifi_IoT";
const char* WIFI_PASSWORD = "VerbindenMetSjeizW1f1_IoT";
const int WIFI_TIMEOUT_SEC = 30;
const int WIFI_RETRY_SEC = 60;
