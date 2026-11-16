# Stiebel PWM Injector 3

## Project Overview

This project is a smart controller for a Stiebel heat pump, built on the NodeMCU ESP8266 platform. It manages pump operation, monitors system status, and provides advanced logging and web-based diagnostics. The controller reads the heat pump's status from the Stiebel ISG over the ISG's HTTP API, reads PWM and temperature sensors, and exposes a web interface for monitoring and configuration.

## Features

- **WiFi Connectivity:** Connects to your local IoT WiFi network with configurable credentials.
- **ISG HTTP API:** Polls web ID 471 OPERATING STATUS and web ID 438 FLOW RATE from the ISG (Stiebel Servicewelt) via `isg_api.php`, the same endpoint and web IDs Home Assistant uses (see "ISG data source" below).
- **PWM Output Control:** Generates a PWM signal to control the pump, with configurable frequency and duty cycle.
- **Sensor Monitoring:** Reads and averages PWM input, flow temperature, and flow rate for stable measurements.
- **Post-Run Timer:** Keeps the pump running for a configurable period after a heating cycle.
- **WebLogger:** Advanced web-based logging with smart filtering, showing only relevant status changes and updates.
- **OTA Updates:** Supports secure over-the-air firmware updates.
- **Time Synchronization:** Syncs time via NTP and supports timezone configuration.

## How It Works

1. **Startup:** The controller connects to WiFi and synchronizes time using NTP.
2. **ISG Polling:** At regular intervals, it asks the ISG API for operating status and flow rate in one non-blocking HTTP request.
3. **Sensor Sampling:** PWM input and temperature sensors are read and averaged for stability.
4. **State Management:** The system tracks pump/compressor states and manages transitions, including post-run timing.
5. **Logging:** Status changes, significant sensor deltas, and periodic updates are logged to the web interface. Filtering ensures only relevant detail lines are shown.
6. **Web Interface:** Users can view real-time status, logs, and configuration via a browser.
7. **OTA Updates:** Firmware can be updated remotely using a secure password.

## Configuration

All main settings are defined in `src/config.cpp`:

- **WiFi:** SSID, password, hostname, timeouts.
- **ISG:** host, HTTP port, API path, request timeout, web IDs, poll interval.
- **PWM:** Output frequency and duty cycle.
- **Post-Run:** Duration after cycle.
- **Time:** Timezone, NTP server, resync interval.
- **WebLogger:** Delta thresholds for temperature, flow, PWM-in; detail interval; page refresh rate.

## ISG data source

The controller reads the ISG through `isg_api.php`, the HTTP API from the stiebel-isg repo that Home
Assistant also reads (`KOPPELING.md` there describes the endpoint). It uses the same web IDs and bit
numbers as HA, where a register is `sensor.isg_<webid>` and a bit is `binary_sensor.isg_<webid>_<bit>`:

| Web ID | Stiebel name (German)              | Unit     | Used for                         |
|--------|------------------------------------|----------|----------------------------------|
| 471    | OPERATING STATUS (BETRIEBSSTATUS)  | bit field | state machine (`statemachine.md`) |
| 438    | FLOW RATE (VOLUMENSTROM)           | l/min    | logging only                     |

The bit constants and their names are in `src/config.h` (`ISG_471_*`); the endpoint settings in
`src/config.cpp` (`ISG_*`).

- **One request per poll** (`GET isg_api.php?ids=471,438`), plain HTTP, read path, no token.
- **HTTP/1.0**, so the ISG answers with a `Content-Length` and closes the connection instead of using
  chunked encoding.
- **Non-blocking:** the ISG takes a few seconds to answer, because the endpoint reads every register
  per call. The controller sends the request and then collects the answer over later loop passes, so
  PWM-in sampling, the web server, telnet and OTA keep running. The answer is parsed with ArduinoJson.
- **No value** -- request failed, timed out (`ISG_HTTP_TIMEOUT_MS`), or the endpoint left a web ID out
  because it held no measurement -- makes 471 read as `ISG_READ_ERROR`, which puts the state machine in
  ERROR.
- The ISG has a single PHP worker: a call from this controller and one from HA are served one after
  the other.

## Smart Logging Criteria

A detail log line is shown when:
- The system state changes (always logged immediately).
- The pump or compressor switches on/off.
- The temperature, flow rate, or PWM-in changes more than the configured delta.
- WiFi status or the ISG operating status (web ID 471) changes.
- The configured detail interval elapses (ensures periodic updates).

## Folder Structure

- `src/` — Main source code (controllers, sensors, loggers, managers)
- `include/` — Header files
- `lib/` — External libraries
- `test/` — Host tests and their ISG test data (`TESTING.md`)
- `platformio.ini` — PlatformIO build configuration

## Getting Started

1. Clone the repository.
2. Open in VS Code with PlatformIO extension.
3. Configure WiFi and ISG settings in `src/config.cpp` as needed.
4. Run the host tests (`pio test -e native`, see `TESTING.md`); all must pass before a flash.
5. Build and upload to your NodeMCU ESP8266 (OTA: see "Flashing over OTA" below).
6. Access the web interface via the device's IP address.

## Flashing over OTA

Flash only with Erik's explicit permission, and only after the host tests pass (`TESTING.md`).

The device sits on the IoT VLAN; the PC that uploads is on the LAN (`host_ip` in `platformio.ini`).
espota sends a UDP invitation to the device (port 8266), after which **the device opens a TCP
connection back to the PC**. The UniFi Dream Machine firewall rule "Block local ip adresses from
vlan:12 (IoT)" blocks that connection, so without an exception the upload fails with
`No response from device`.

Procedure:
1. Add a temporary, narrow exception on the UDM: only between the device and the uploading PC,
   both directions, in the live firewall (not in the UniFi configuration). Over SSH
   (`ssh -i C:/Users/ErikCheizoo/.ssh/ha_sshfs root@192.168.1.1`):
   ```
   iptables -I FORWARD 1 -s <device-ip> -d <pc-ip> -m comment --comment claude-ota-temp -j ACCEPT
   iptables -I FORWARD 1 -s <pc-ip> -d <device-ip> -m comment --comment claude-ota-temp -j ACCEPT
   ```
   Device and PC addresses: `upload_port` and `host_ip` in `platformio.ini`.
2. Upload: `pio run -e nodemcuv2-ota -t upload`.
3. Remove the exception straight away, also when the upload failed (the same two rules with
   `-D` instead of `-I 1`), and check that `iptables -S FORWARD | grep -c claude-ota-temp` is 0.
   A UDM reboot or re-provision also drops it; the block rule for the rest of the IoT VLAN stays
   active throughout.
4. Check the web log (`/log`): after the restart the state line shows `ISG471:` with a valid
   status value and the expected state within about a minute.

A failed upload leaves the old firmware running: the new image only becomes active after it has
arrived completely and verified.

## Requirements

- NodeMCU ESP8266
- PlatformIO (VS Code)
- Stiebel ISG with the `isg_api.php` endpoint installed (stiebel-isg repo)
- WiFi network

## Security

- OTA updates require a password (`OTA_PASSWORD` in config).
- WiFi credentials are stored in code; secure your network accordingly.

## License

See LICENSE file for details.

## Author

Sjeiz / Cheizoo
