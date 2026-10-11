# Pneuma Firmware

Screen-free breathing device. Part of Back to Boring.

## Status

Power-on, e-paper display, exercise menu, breathing pacer animation, battery
monitoring, deep sleep. The breathing pacer
currently runs on fixed per-phase timers, one set per exercise, defined in
`src/app/exercises.cpp`.

**Controls**: BTN_A = select / confirm / long-press = back or cancel.
BTN_B = cycle through the menu. IDLE → (BTN_A) → MENU → (BTN_A on a
highlighted exercise) → BREATHING.

**Exercises included** (Box, 4-7-8, Coherent) are common reference timings,
not yet confirmed as your intended exercises — check `src/app/exercises.cpp`
and adjust freely; the menu and breathing runner both read from that file
alone, nothing elsewhere needs to change when you edit it.

## Build setup (Arduino IDE)

- Board package: **esp32 by Espressif Systems** — use a version that
  supports OPI PSRAM boot config for the ESP32-S3-PICO-1-N8R8.
- Board: ESP32S3 Dev Module
- Board menu settings to set explicitly:
  - PSRAM: **OPI PSRAM**
  - Partition Scheme: (document your chosen scheme here once decided)
  - Flash Size / Upload Speed: (fill in from your working config)

## Library dependencies (Arduino Library Manager)

- Adafruit GFX Library

Fonts used (`FreeSans9pt7b`, `FreeSansBold9pt7b`, `FreeSansBold12pt7b`,
`FreeSansBold18pt7b`) ship with Adafruit GFX.

## Editing in VS Code

Compile/upload stays in the Arduino IDE. VS Code is used as the editor;
IntelliSense red squiggles on ESP-IDF headers (`driver/gpio.h` etc.) are
expected and harmless unless you install an Arduino IntelliSense extension.

## Known open items

1. **RTC chip verification**: `rtc.h`/`rtc.cpp` assume a PCF85063-compatible
   register map. Confirm this matches the actual RTC IC on your board before
   trusting time readings — see the note next to `I2C_RTC_DEV_Address` in
   `config.h`.
2. **BTN_B pin placeholder**: currently mapped to GPIO0 (BOOT pin) in
   `config.h` as a placeholder. Confirm this is safe on your wiring before
   final hardware, or move it to a dedicated pin.
3. **Battery fix applied**: see comments in
   `src/power/board_power_bsp.cpp` (constructor) and `pneuma.ino` (top of
   `setup()`) for the fix to the battery-only cold-boot power-loss issue.
4. **Leitgedanke content**: `LEITGEDANKE` in `src/app/ui.cpp` is a hardcoded
   placeholder. Swap in the `prompt_today.json` pipeline once designed.
5. **Exercise timings unconfirmed**: see `src/app/exercises.cpp` — check
   the three included patterns against what you actually intended.

## Structure

```
pneuma.ino
config.h / globals.h / types.h
src/
  power/     — VBAT + EPD power control (battery-fix applied here)
  i2c_bsp/   — I2C bus + RTC device handle
  display/   — Waveshare 1.54" e-paper driver (ported, unmodified)
  app/       — draw (graphics primitives), battery, buttons, rtc, sleep,
               exercises (breathing pattern data), ui (screens)
```
