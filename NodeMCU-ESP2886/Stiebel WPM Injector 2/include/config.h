#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuratie
const char* WIFI_SSID = "SjeizWifi_IoT";
const char* WIFI_PASSWORD = "VerbindenMetSjeizW1f1_IoT";

// ISG-web Modbus TCP configuratie
const char* ISG_HOST = "servicewelt.iot.cheizoo.lan";
const uint16_t ISG_PORT = 502;
const uint8_t ISG_SLAVE_ID = 1;

// Timing configuratie
const unsigned long READ_INTERVAL = 15000; // 15 seconden

// PWM configuratie (voor D5 post-run timer)
const uint16_t PWM_FREQUENCY = 150;  // 150 Hz
const uint8_t PWM_DUTY_CYCLE = 30;   // 30% duty cycle

// Debug configuratie
const bool DEBUG_ENABLED = false; // Zet op true voor debug logging

#endif // CONFIG_H
