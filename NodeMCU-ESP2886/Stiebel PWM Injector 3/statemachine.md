
# Stiebel WPM3 Statemachine

Source of truth: `centralTransition()` in `src/classes/StateManager.cpp` (states) and
`OutputManager::loop()` in `src/classes/OutputManager.cpp` (outputs).

## Input

The input is ISG web ID **471 OPERATING STATUS** (BETRIEBSSTATUS), a bit field, read from the ISG's
HTTP API (`isg_api.php`) -- the same endpoint, web ID and bit numbers Home Assistant uses
(`sensor.isg_471`, `binary_sensor.isg_471_<bit>`). The bit constants are `ISG_471_*` in
`src/config.h`, each with its Stiebel name.

| Bit | Stiebel name (German)        | HA entity                  | Used for                 |
|-----|------------------------------|----------------------------|--------------------------|
| 4   | HEATING (HEIZBETRIEB)        | `binary_sensor.isg_471_4`  | HEATING                  |
| 5   | DHW MODE (WARMWASSERBETRIEB) | `binary_sensor.isg_471_5`  | HOT_WATER                |
| 6   | COMPRESSOR (VERDICHTER)      | `binary_sensor.isg_471_6`  | compressor on/off        |
| 7   | SUMMER MODE (SOMMERBETRIEB)  | `binary_sensor.isg_471_7`  | post-run on/off          |
| 8   | COOLING MODE (KUEHLBETRIEB)  | `binary_sensor.isg_471_8`  | COOLING                  |
| 9   | DEFROST (ABTAUBETRIEB)       | `binary_sensor.isg_471_9`  | DEFROST                  |

"No value" (`ISG_READ_ERROR`) means the request failed or timed out, or the endpoint left 471 out
of its payload because it held no measurement (HA shows that as `unknown`).

## Transitions

Evaluated on every loop, first match wins:

1. **ERROR**: 471 has no value (`ISG_READ_ERROR`). Immediate, cancels the post-run timer. Also the start state after boot.
2. **DEFROST**: 471 bit 9 DEFROST=1, regardless of the compressor bit. Immediate, cancels the post-run timer.
3. **Minimum state time**: any other change waits until the current state has lasted `ISG_POLL_STABLETIME_SEC`. Not applied when leaving ERROR.
4. **HOT_WATER → POST_RUN**: compressor (bit 6) turns off while in HOT_WATER **and** bit 7 SUMMER MODE=0. Starts the post-run timer (`POST_RUN_DURATION_MIN`). With bit 7=1 the next rule applies and the state becomes STANDBY.
5. **In POST_RUN**: bit 8 COOLING MODE, bit 4 HEATING or bit 5 DHW MODE set (compressor bit not checked) → cancel the timer and go to COOLING, HEATING or HOT_WATER, in that order. Otherwise stay until the timer expires, then STANDBY.
6. **STANDBY**: compressor (bit 6) off.
7. Compressor on: bit 8 COOLING MODE → **COOLING**, else bit 4 HEATING → **HEATING**, else bit 5 DHW MODE → **HOT_WATER**, else **STANDBY**.

## Outputs per state

Outputs are only written when the state changes.

| State                           | D7 PUMP_FORCE | D8 PUMP_BLOCKED | D5 PWM-out                                   | Log label (PumpHK2) |
|---------------------------------|---------------|-----------------|----------------------------------------------|---------------------|
| ERROR, STANDBY, HEATING, HOT_WATER | LOW        | LOW             | not driven: WPM3 signal passes through       | NORMAL              |
| DEFROST                         | LOW           | LOW             | not driven                                   | NORMAL              |
| COOLING                         | HIGH          | LOW             | not driven: WPM3 signal passes through       | FORCED              |
| POST_RUN                        | HIGH          | LOW             | injects `PWM_OUT_DUTY_PERCENT` (inverted, open collector) | FORCED   |

Notes:
- DEFROST deliberately does not block the pump (D8 stays LOW): blocking makes the heat pump enable the electric heater (NHZ). It only changes the LED pattern.
- The log label is read back from the pins: BLOCKED if D8 is HIGH, FORCED if D7 is HIGH, otherwise NORMAL.

## Logging

- Every state change is logged immediately to all loggers (serial, telnet, web), with the time spent in the previous state.
- `LogManager` produces a status line every `statusInterval` (`src/classes/LogManager.h`). Serial and telnet write every line; the web logger only writes it when state, pump label, compressor, WiFi or the 471 value change, when PWM-in, flow temperature or flow rate move more than their `WEBLOGGER_*_DELTA*` thresholds, or at least once per `WEBLOGGER_DETAIL_INTERVAL_SEC`.
- The web log is a ring buffer of `WEBLOGGER_LINES_COUNT` lines (`src/loggers/WebLogger.h`), shown newest first; the oldest lines are overwritten.
