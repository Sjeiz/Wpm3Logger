

#include <Arduino.h>
#include <time.h>
#include "config.h"




// Logging configuration
const bool DEBUG   = true;
const bool VERBOSE = false;

// WiFi credentials
const char* HOSTNAME       = "stiebelpumpcontrol";
const char* WIFI_SSID      = "SjeizWifi_IoT";
const char* WIFI_PASSWORD  = "VerbindenMetSjeizW1f1_IoT";
const int WIFI_TIMEOUT_SEC = 30;
const int WIFI_RETRY_SEC   = 60;

// OTA password configuration
const char* OTA_PASSWORD = "StiebelPumpControl";

// Modbus/ISG configuration
const char* ISG_HOST                = "servicewelt.iot.cheizoo.lan";
const int ISG_PORT                  = 502;
const int ISG_OPERATING_STATUS_ADDR = 2500;
const int ISG_POLL_INTERVAL_SEC     = 10;
const ModbusRegDef MODBUS_REGS[]    = {	{"OPERATING_STATUS", 2500},	{"FLOW_RATE", 520}};
const ModbusConfig MODBUS_CONFIG    = {MODBUS_REGS, sizeof(MODBUS_REGS) / sizeof(ModbusRegDef)};


// PWM configuration
const int PWM_OUT_FREQUENCY_HZ = 150;
const int PWM_OUT_DUTY_PERCENT = 25; // Default output PWM duty cycle percentage

// Post-run timer
const int POST_RUN_DURATION_MIN = 20; // Minutes the pump keeps running after a cycle

// Time configuration
const char* TIMEZONE              = "CET-1CEST,M3.5.0/2,M10.5.0/3"; // Europe/Amsterdam
const char* NTP_SERVER            = "pool.ntp.org";
const int NTP_RESYNC_INTERVAL_MIN = 60;

// WebLogger configuration
// Note: WEBLOGGER_LINES_COUNT must be defined in WebLogger.h due to compiler constraints
// Note: WEBLOGGER_LINES_LENGTH must be defined in WebLogger.h due to compiler constraints
const float WEBLOGGER_TEMP_DELTA         = 0.2; // Temperature difference delta for logging (°C)
const float WEBLOGGER_FLOW_DELTA_PCT     = 5; // Flow rate difference delta for logging (%)
const float WEBLOGGER_PWMIN_DELTA_PCT    = 20; // PWM input difference delta for logging (%)
const int WEBLOGGER_DETAIL_INTERVAL_SEC  = 600;  // At least every xxx seconds a detail line
const int WEBLOGGER_REFRESH_PAGE_SEC     = ISG_POLL_INTERVAL_SEC / 2;  // WebLogger page refresh interval (seconds)
