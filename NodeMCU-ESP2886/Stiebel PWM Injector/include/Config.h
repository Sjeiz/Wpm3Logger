#ifndef CONFIG_H
#define CONFIG_H

// ===== CONFIGURATION =====
// Debug
#define DEBUG_MODE false  // Set to true to enable debug output

// Logging
#define LOG_INTERVAL 30000  // Log every 30 seconds (milliseconds)

// PWM Output (only active during post-run timer)
#define PWM_OUTPUT_FREQ_POSTRUN 150        // Post-run timer frequency (Hz)
#define PWM_OUTPUT_DUTY_POSTRUN 30         // Post-run timer duty cycle (%)

// Post-Run Timer
#define POSTRUN_TIMER_DURATION (30 * 60 * 1000)   // Post-run timer duration (30 minutes)

// PWM Detection
#define PWM_DEBOUNCE_TIME 3000   // Debounce time in microseconds (3ms)
#define PWM_DETECTION_TIMEOUT 5000  // 5 seconds without edge = no PWM

// WiFi
#define WIFI_CONNECTION_TIMEOUT 30000      // 30 seconds max wait for initial connection
#define WIFI_RECONNECT_INTERVAL 60000      // Try reconnect every 60 seconds if disconnected
#define NTP_RESYNC_INTERVAL 60000          // Try NTP resync every 60 seconds if time not valid
#define NTP_RESYNC_INTERVAL_VALID 3600000  // Resync every hour when time is valid

// LED Blinking
#define LED_BLINK_STANDBY 200UL  // Standby blink interval (ms, short flashes)
#define LED_BLINK_SLOW 500UL     // Slow blink interval (ms, PWM-in active)
#define LED_BLINK_FAST 100UL     // Fast blink interval (ms, post-run timer active)

// Serial & Web
#define SERIAL_BAUD_RATE 115200          // Serial communication baud rate
#define WEBSERVER_REFRESH_INTERVAL 30    // Web page refresh interval (seconds)
#define PWM_TEST_DURATION 2000           // PWM test duration during setup (milliseconds)
#define WEB_LOG_BUFFER_SIZE 100          // Number of log lines to keep in web buffer
#define HEAP_WARNING_THRESHOLD 12000     // Warn when free heap drops below this (bytes)

// WiFi Credentials
#define WIFI_SSID "SjeizWifi_IoT"
#define WIFI_PW   "VerbindenMetSjeizW1f1_IoT"

// PIN DEFINITIONS
#define PIN_PWM_OUT D5          // PWM output
#define PIN_PWM_IN  D6          // PWM input (optocoupler)
#define PIN_PUMP    D7          // Pump HK2 control
#define LED_PIN     LED_BUILTIN // Onboard LED (active LOW)

#endif
