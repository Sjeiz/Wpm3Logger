
#include <Arduino.h>
#include <time.h>
#include "config.h"




// Logging configuration
const bool DEBUG = false;
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
const int ISG_PORT = 502;
const int ISG_OPERATING_STATUS_ADDR = 2500;
const int ISG_POLL_INTERVAL_SEC = 5;
const ModbusRegDef MODBUS_REGS[] = {
	{"OPERATING_STATUS", 2500},
	{"FLOW_RATE", 520}
};
const ModbusConfig MODBUS_CONFIG = {MODBUS_REGS, sizeof(MODBUS_REGS) / sizeof(ModbusRegDef)};


// PWM configuration
const int PWM_OUT_FREQUENCY_HZ = 150;
const int PWM_OUT_DUTY_PERCENT = 30;

// Post-run timer
const int POST_RUN_DURATION_MIN = 1;

// Time configuration
const char* TIMEZONE = "CET-1CEST,M3.5.0/2,M10.5.0/3"; // Europe/Amsterdam
const char* NTP_SERVER = "pool.ntp.org";
const int NTP_RESYNC_INTERVAL_MIN = 60;

// WebLogger configuration
const int WEBLOGGER_BUFFER_SIZE = 8192;  // Approx. 50 lines
const float WEBLOGGER_TEMP_MARGE  = 0.2; // Temperatuur verschil marge voor logging
const int WEBLOGGER_DETAIL_INTERVAL_MIN = 1; // Minimaal elke 15 minuten een detailregel
