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
const unsigned long READ_INTERVAL = 30000; // 30 seconden

#endif // CONFIG_H
