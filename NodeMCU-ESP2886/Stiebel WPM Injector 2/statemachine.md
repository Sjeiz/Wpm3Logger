
# Stiebel WPM3 Statemachine

| State      | Conditions (enabled bits)         | PUMP_STATUS | PUMP_ON (D7) | PUMP_BLOCKED (D8) | Comments                                      |
|------------|-----------------------------------|-------------|--------------|-------------------|-----------------------------------------------|
| ERROR      | WiFi or Modbus error              | NORMAL      | LOW          | LOW               | WiFi or Modbus error. Pumps solely controlled by heatpump |
| STANDBY    | B6:COMPRESSOR=0                   | NORMAL      | LOW          | LOW               | Compressor off                                |
| DEFROST    | B6:COMPRESSOR=1, B9:DEFROST=1     | BLOCKED     | LOW          | HIGH              | Always priority if B9=1                       |
| COOLING    | B6:COMPRESSOR=1, B8:COOLING=1     | NORMAL      | LOW          | LOW               | Only if B9=0                                  |
| HOT_WATER  | B6:COMPRESSOR=1, B5:HOT_WATER=1   | NORMAL      | LOW          | LOW               | Only if B9=0 and B8=0                         |
| HEATING    | B6:COMPRESSOR=1, B4:HEATING=1     | NORMAL      | LOW          | LOW               | Only if B9=0, B8=0, B5=0                      |
| POST_RUN   | HOT_WATER → STANDBY               | FORCED      | HIGH         | LOW               | Only active in STANDBY, immediately cancelled if status is no longer STANDBY |


Notes:
- POST_RUN starts on transition HOT_WATER → STANDBY.
- POST_RUN may only be active if the main status is STANDBY.
- As soon as the status is no longer STANDBY, POST_RUN is immediately cancelled.
- DEFROST (B9) always has priority if compressor is on.
- Bit numbers: B4=HEATING, B5=HOT_WATER, B6=COMPRESSOR, B8=COOLING, B9=DEFROST.
