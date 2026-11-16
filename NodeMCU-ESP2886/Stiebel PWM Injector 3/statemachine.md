
# Stiebel WPM3 Statemachine

Source of truth: `centralTransition()` in `src/classes/StateManager.cpp` (states) and
`OutputManager::loop()` in `src/classes/OutputManager.cpp` (outputs). Input is the ISG
operating status register; bit names are the `ISG_STATUS_*` constants in `src/config.h`.

## Transitions

Evaluated on every loop, first match wins:

1. **ERROR**: status equals `ISG_MODBUS_READ_ERROR` (Modbus read failed). Immediate, cancels the post-run timer. Also the start state after boot.
2. **DEFROST**: B9 DEFROSTING=1, regardless of the compressor bit. Immediate, cancels the post-run timer.
3. **Minimum state time**: any other change waits until the current state has lasted `ISG_POLL_STABLETIME_SEC`. Not applied when leaving ERROR.
4. **HOT_WATER → POST_RUN**: compressor (B6) turns off while in HOT_WATER **and** B7 SUMMER_MODE_ACTIVE=0. Starts the post-run timer (`POST_RUN_DURATION_MIN`). With B7=1 the next rule applies and the state becomes STANDBY.
5. **In POST_RUN**: B8 COOLING, B4 HEATING or B5 HOT_WATER set (compressor bit not checked) → cancel the timer and go to COOLING, HEATING or HOT_WATER, in that order. Otherwise stay until the timer expires, then STANDBY.
6. **STANDBY**: compressor (B6) off.
7. Compressor on: B8 COOLING → **COOLING**, else B4 HEATING → **HEATING**, else B5 HOT_WATER → **HOT_WATER**, else **STANDBY**.

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
- `LogManager` produces a status line every `statusInterval` (`src/classes/LogManager.h`). Serial and telnet write every line; the web logger only writes it when state, pump label, compressor, WiFi or Modbus value change, when PWM-in, flow temperature or flow rate move more than their `WEBLOGGER_*_DELTA*` thresholds, or at least once per `WEBLOGGER_DETAIL_INTERVAL_SEC`.
- The web log is a ring buffer of `WEBLOGGER_LINES_COUNT` lines (`src/loggers/WebLogger.h`), shown newest first; the oldest lines are overwritten.
