# ---






## State, Conditions, Pump, PWM-out, PUMP_ON (D7), PUMP_BLOCK (D8), Remarks

| State                  | Conditions (Modbus bits)                | Pump    | PWM-out | PUMP_ON (D7) | PUMP_BLOCK (D8) | Remarks                                         |
|------------------------|-----------------------------------------|---------|---------|--------------|----------------|-------------------------------------------------|
| STANDBY                | COMPRESSOR=0                            | NORMAL  | OFF     | LOW          | LOW            |                                                 |
| HEATING                | COMPRESSOR=1, HEATING=1                 | NORMAL  | OFF     | LOW          | LOW            |                                                 |
| HEATING (defrosting)   | COMPRESSOR=1, HEATING=1, DEFROST=1      | BLOCKED | OFF     | LOW          | HIGH           |                                                 |
| HOT_WATER              | COMPRESSOR=1, HOT_WATER=1               | NORMAL  | OFF     | LOW          | LOW            |                                                 |
| HOT_WATER (defrosting) | COMPRESSOR=1, HOT_WATER=1, DEFROST=1    | BLOCKED | OFF     | LOW          | HIGH           |                                                 |
| COOLING                | COMPRESSOR=1, COOLING=1                 | NORMAL  | OFF     | LOW          | LOW            |                                                 |
| POST_RUN               | Na HOT_WATER→STANDBY, COMPRESSOR=0      | FORCED  | 30%     | HIGH         | LOW            | Wordt geannuleerd als de compressor gaat lopen  |

Toelichting:
- Pump = BLOCKED tijdens defrost (alleen bij HEATING of HOT_WATER)
- Pump = FORCED tijdens post run (na HOT_WATER)
- Pump = NORMAL in alle andere gevallen
# Stiebel WPM Injector Statemachine

| State      | Modbus bits (voorwaarde)         | Defrost property | Post Run gedrag                                            | Opmerking                                   |
|------------|----------------------------------|------------------|------------------------------------------------------------|----------------------------------------------|
| STANDBY    | COMPRESSOR=0                     | nee              | -                                                          | Ruststand                                   |
| HEATING    | COMPRESSOR=1, HEATING=1          | ja/nee           | -                                                          | ja = HEATING (defrosting), nee = normaal     |
| HOT_WATER  | COMPRESSOR=1, HOT_WATER=1        | ja/nee           | -                                                          | ja = HOT_WATER (defrosting), nee = normaal   |
| COOLING    | COMPRESSOR=1, COOLING=1          | nee              | -                                                          | Koelbedrijf                                 |
| POST_RUN   | Na HOT_WATER→STANDBY, COMPRESSOR=0 | nee              | Start bij HOT_WATER→STANDBY, max 30 min, cancel als compressor start | Pomp naloop                                 |

- Defrost property (inDefrostCycle/bit 9) kan alleen actief zijn tijdens HEATING of HOT_WATER.
- POST_RUN volgt alleen na HOT_WATER.
- Bitnummers: COMPRESSOR=6, HEATING=4, HOT_WATER=5, COOLING=8, DEFROST=9.
