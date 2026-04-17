# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ESP8266 (ESP-12E) firmware for a battery-powered water leak sensor. Detects leaks via a digital sensor pin, periodically monitors battery voltage, sends SMS/email notifications through an external HTTPS API, and publishes battery + leak state to MQTT with Home Assistant auto-discovery.

## Build / Upload

`arduino-cli` must be on PATH — invoke it directly, do **not** alias it or use the `/Applications/Arduino IDE.app/...` path.

Preferred workflow:

```bash
scripts/upload.sh            # upload + monitor
scripts/upload.sh --compile  # compile first, then upload
scripts/monitor.sh           # monitor only
```

Both scripts prompt for the serial port on first use and remember it for the rest of the shell session via `$WATER_LEAK_SENSOR_UPLOAD_PORT` (also cached in a per-tty tmp file).

Manual compile:

```bash
arduino-cli compile --fqbn esp8266:esp8266:generic --libraries ~/Documents/Arduino/libraries .
```

IDE settings if using Arduino IDE directly: Generic ESP8266 Module, 115200 upload speed, 4M (1M SPIFFS).

The project is a flat sketch folder. All `.ino`/`.h`/`.cpp` files compile as one sketch: `water_leak_sensor.ino`, `melody.h/cpp`, `pithces.h` (sic — do not "fix" the misspelling), `mqtt.h/cpp`.

## Credentials

The sketch includes `<credentials.h>` — a user-scoped Arduino library at `~/Documents/Arduino/libraries/credentials/credentials.h` (not in this repo). Required macros: `STASSID`, `STAPSK`, `API_HOST`, `API_TOKEN`, `PHONE_NUM`, `EMAIL`, and (for MQTT) `MQTT_SERVER`, `MQTT_PORT`, optionally `MQTT_USER` + `MQTT_PASS`. `PLACE` (e.g. `"bathroom"`) is a `#define` at the top of the sketch, not in credentials — change per-unit.

## Architecture

The firmware is a one-shot setup-only program — `loop()` is intentionally empty. Every wake runs `setup()` end-to-end and then calls `ESP.deepSleep(SLEEP_TIME)`. There is no persistent state across wakes (no RTC memory yet).

Lifecycle:

1. `findOutResetReason()` reads `SENSOR_PIN` (GPIO12): `HIGH` → `STANDARD_WAKEUP` (timed voltage check), `LOW` → `ALARM_WAKEUP` (water bridging the probes pulled the pin low).
2. **Alarm wake**: connect WiFi → send SMS + email via HTTPS → publish `leak=ON` + battery to MQTT → loop the alarm melody while the pin stays low.
3. **Standard wake**: read voltage (100-sample A0 average through the 1kΩ/100Ω divider, calibration coefficient `0.985`) → connect WiFi → if voltage < `CRITICAL_VOLTAGE` (3.6 V) send low-battery email + beep → publish `leak=OFF` + battery to MQTT.
4. `ESP.deepSleep(3h)`.

Every wake connects WiFi. This is a deliberate cost (~2s of 70mA every 3h) in exchange for continuous battery visibility in Home Assistant and prompt clearing of a stale retained `leak=ON` after a recovered alarm.

### MQTT layer (`mqtt.h`/`mqtt.cpp`)

Deep-sleep-friendly: connect → publish → disconnect inline during the wake. `initiateMqtt(place)` builds topic strings from the chip ID + place at runtime (so multiple units share one firmware). Discovery is published on every wake — the broker retains discovery configs so this is effectively idempotent, and it self-heals if the broker loses retained state.

Home Assistant device: one device per chip, id `water_leak_<chipid>`, with two entities — `sensor.battery` (device_class `voltage`, unit V) and `binary_sensor.leak` (device_class `moisture`). Availability uses LWT on `<base>/available`.

Topic shape: `water_leak_sensor/<place>/{battery,leak,available}`.

HTTPS notifications (SMS + email) remain in place — they use `WiFiClientSecure::setInsecure()` (no cert validation) with JSON bodies concatenated as `String`.

### Pin map

| Symbol       | GPIO | Role                                            |
|--------------|------|-------------------------------------------------|
| `LED_PIN`    | 2    | Status LED (driven HIGH in setup)               |
| `SENSOR_PIN` | 12   | Water probe input; LOW = leak                   |
| `BUZZER_PIN` | 13   | Piezo (defined in `melody.h`)                   |
| `A0`         | —    | Battery voltage via 1kΩ/100Ω divider            |

## Gotchas

- Deep sleep requires GPIO16 tied to RST; the sensor wakes via RST as well, so the sensor circuit must pulse RST on leak in addition to pulling GPIO12 low (see `circuit_diagram/image.png` / `easyeda_source_code.json`).
- `callibrationCoefficient` in `readVoltage()` is device-specific (cheap ADC) — expect to retune per unit. Currently hard-coded, not in EEPROM.
- If you change the battery chemistry/voltage, adjust `CRITICAL_VOLTAGE` accordingly (README notes input range 3.5–12 V).
- `DEBUG` is a compile-time `#define` at the top of the sketch — uncomment to enable Serial logging at 115200 baud.
- The filename `pithces.h` is misspelled; preserve the spelling when including it.
- `PubSubClient`'s default 256-byte buffer is too small for HA discovery payloads; `initiateMqtt()` bumps it to 512. Keep that in mind if you add more entities.
- Standard wake now always connects WiFi (even when voltage is fine) to publish the periodic battery reading. This slightly increases per-wake power use vs. the original firmware.
