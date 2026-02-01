# Stiebel PWM Injector 3

## Project Overview

This project is a smart controller for a Stiebel heat pump, built on the NodeMCU ESP8266 platform. It manages pump operation, monitors system status, and provides advanced logging and web-based diagnostics. The controller communicates with the heat pump via Modbus TCP, reads PWM and temperature sensors, and exposes a web interface for monitoring and configuration.

## Features

- **WiFi Connectivity:** Connects to your local IoT WiFi network with configurable credentials.
- **Modbus TCP Communication:** Polls operating status and flow rate from the ISG (Stiebel Servicewelt) using Modbus TCP.
- **PWM Output Control:** Generates a PWM signal to control the pump, with configurable frequency and duty cycle.
- **Sensor Monitoring:** Reads and averages PWM input, flow temperature, and flow rate for stable measurements.
- **Post-Run Timer:** Keeps the pump running for a configurable period after a heating cycle.
- **WebLogger:** Advanced web-based logging with smart filtering, showing only relevant status changes and updates.
- **OTA Updates:** Supports secure over-the-air firmware updates.
- **Time Synchronization:** Syncs time via NTP and supports timezone configuration.

## How It Works

1. **Startup:** The controller connects to WiFi and synchronizes time using NTP.
2. **Modbus Polling:** At regular intervals, it polls the ISG for operating status and flow rate.
3. **Sensor Sampling:** PWM input and temperature sensors are read and averaged for stability.
4. **State Management:** The system tracks pump/compressor states and manages transitions, including post-run timing.
5. **Logging:** Status changes, significant sensor deltas, and periodic updates are logged to the web interface. Filtering ensures only relevant detail lines are shown.
6. **Web Interface:** Users can view real-time status, logs, and configuration via a browser.
7. **OTA Updates:** Firmware can be updated remotely using a secure password.

## Configuration

All main settings are defined in `src/config.cpp`:

- **WiFi:** SSID, password, hostname, timeouts.
- **Modbus:** ISG host, port, register addresses, poll interval.
- **PWM:** Output frequency and duty cycle.
- **Post-Run:** Duration after cycle.
- **Time:** Timezone, NTP server, resync interval.
- **WebLogger:** Delta thresholds for temperature, flow, PWM-in; detail interval; page refresh rate.

## Smart Logging Criteria

A detail log line is shown when:
- The system state changes (always logged immediately).
- The pump or compressor switches on/off.
- The temperature, flow rate, or PWM-in changes more than the configured delta.
- WiFi or Modbus connection status changes.
- The configured detail interval elapses (ensures periodic updates).

## Folder Structure

- `src/` — Main source code (controllers, sensors, loggers, managers)
- `include/` — Header files
- `lib/` — External libraries
- `test/` — Test code
- `platformio.ini` — PlatformIO build configuration

## Getting Started

1. Clone the repository.
2. Open in VS Code with PlatformIO extension.
3. Configure WiFi and ISG settings in `src/config.cpp` as needed.
4. Build and upload to your NodeMCU ESP8266.
5. Access the web interface via the device's IP address.

## Requirements

- NodeMCU ESP8266
- PlatformIO (VS Code)
- Stiebel ISG with Modbus TCP enabled
- WiFi network

## Security

- OTA updates require a password (`OTA_PASSWORD` in config).
- WiFi credentials are stored in code; secure your network accordingly.

## License

See LICENSE file for details.

## Author

Sjeiz / Cheizoo
