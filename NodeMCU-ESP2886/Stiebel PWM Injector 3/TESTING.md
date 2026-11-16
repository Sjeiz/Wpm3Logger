# Testing

## Before any flash

Every firmware build that goes to the controller first passes the host test suite, with all cases
green. The suite runs the firmware's own code -- `IsgApiManager` (request, receive, parse),
`centralTransition()`/`StateManager` (states) and `OutputManager` (pins) -- in the same order as
`loop()` in `src/main.cpp`, against ISG API responses served by a scripted fake ISG. It must cover:

1. **Live responses compared with Home Assistant.** Responses captured from the ISG, parsed by the
   controller, compared with what HA showed for the same web IDs at the same moment: the register
   values (`sensor.isg_471`, `sensor.isg_438`, `sensor.isg_402`, `sensor.isg_485`) and the bits
   (`binary_sensor.isg_471_<bit>`, `isg_402_<bit>`, `isg_485_<bit>`), plus the state the live
   response drives the state machine into.
2. **Every state, transition and output state**, with synthetic responses: a live capture with only
   the 471 value changed. Start-up, ERROR and recovery, the minimum state time, DEFROST (immediate,
   passive, cancels the post-run), HEATING, HOT_WATER → POST_RUN outside summer mode and → STANDBY in
   summer mode, the POST_RUN timer and its three cancel paths with their priority, COOLING with D7 on
   and no PWM, the compressor-on priority and its fallback, and D8 never set.
3. **Failures lead to passive behaviour.** HTTP errors, a timeout, a refused connection, an empty or
   oversized answer, a body that is not a JSON object, and a 471 field that is missing or invalid
   (text, fraction, negative, out of 16 bits, null, object) must each give "no value" → ERROR → D7
   and D8 low, no PWM.

Which case covers which path is in the comments of `test/test_native/test_main.cpp`.

A suite that passes is only evidence if it can fail: after changing the state machine or the parser,
break the changed rule on purpose once and check that a test catches it.

## How to run

```
pio test -e native
```

Runs on the PC in a few seconds; it needs a host C++ compiler (g++) on the PATH. It builds only the
sources listed in `build_src_filter` of `[env:native]` in `platformio.ini`, against the stand-ins in
`test/shim/arduino/` (Arduino core, `ESP8266WiFi` with the scripted fake ISG, `Ticker`). The firmware
build itself is `pio run -e nodemcuv2-ota`; neither command uploads anything.

## Test data

- `test/test_native/fixtures/live_*.http`: raw HTTP responses captured from the ISG with the
  controller's request form (`GET /isg_api.php?ids=... HTTP/1.0`), headers and body byte for byte.
  The capture time is in the file name and the `Date` header. They hold no credentials: the read path
  of the API needs no token.
- `test/test_native/fixtures/.gitattributes` keeps their CRLF line endings intact.
- The HA values they are compared with are in the test that uses them, with the entity IDs and the
  moment.
- Synthetic responses are built in the test from a live capture (`withStatus()`, `withField()`,
  `withBody()`), so they keep the real headers and only differ in what the case is about.

A new capture: `curl -s --http1.0 -i "http://<isg>/isg_api.php?ids=471,438,402,485" > fixtures/live_<UTC time>_<state>.http`,
then read the HA values for that moment from the HA history and add a case.
