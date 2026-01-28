// Only include the required headers for time/NTP on ESP8266
#include <Arduino.h>
#include <time.h>
#include "config.h"

// Logging configuration
const bool DEBUG = true;
const bool VERBOSE = false;

// WiFi credentials
const char* HOSTNAME = "stiebelpumpcontrol";
const char* WIFI_SSID = "SjeizWifi_IoT";
const char* WIFI_PASSWORD = "VerbindenMetSjeizW1f1_IoT";
const int WIFI_TIMEOUT_SEC = 30;
const int WIFI_RETRY_SEC = 60;

// OTA password configuration
const char* OTA_PASSWORD = "StiebelPumpControl";

// Modbus/ISG configuration
const char* ISG_HOST = "servicewelt.iot.cheizoo.lan";
const int ISG_MODBUS_PORT = 502;
const int ISG_OPERATING_STATUS_ADDR = 2500;

// PWM configuration
const int PWM_OUT_FREQUENCY_HZ = 150;
const int PWM_OUT_DUTY_PERCENT = 30;

// Post-run timer
const int POST_RUN_DURATION_MIN = 1;

// Time configuration
const char* TIMEZONE = "CET-1CEST,M3.5.0/2,M10.5.0/3"; // Europe/Amsterdam
const char* NTP_SERVER = "pool.ntp.org";
const int NTP_RESYNC_INTERVAL_MIN = 60;

// WebLogger buffer size
const int WEBLOGGER_BUFFER_SIZE = 8192;
