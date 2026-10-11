#include "Arduino.h"
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"

extern "C" {
#include "config.h"
#include "src/i2c_bsp/i2c_bsp.h"
}

#include "src/power/board_power_bsp.h"
#include "src/display/epaper_driver_bsp.h"

#include <Adafruit_GFX.h>
#include <pgmspace.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "types.h"
#include "globals.h"
#include "src/app/draw.h"
#include "src/app/battery.h"
#include "src/app/rtc.h"
#include "src/app/exercises.h"
#include "src/app/ui.h"
#include "src/app/buttons.h"
#include "src/app/sleep.h"

// ─── Global variable definitions ───────────────────────────────────────────
//
// Battery fix, part 1: `board` is a global object, constructed here before
// setup() runs at all. Its constructor (board_power_bsp.cpp) already
// latches VBAT power immediately — the earliest point any of our code can
// execute. See the comment there for the full explanation.
board_power_bsp_t      board(EPD_PWR_PIN, VBAT_PWR_PIN);
epaper_driver_display* display = nullptr;

AppState state = STATE_IDLE;

uint32_t lastActivityMs     = 0;
bool     wokeFromUltraSleep = false;

uint32_t lastBatCheckMs = 0;
bool     batLowWarned   = false;

bool timeReady = false;

// ─── Menu state ──────────────────────────────────────────────────────────
static int menuCursor = 0;

// ─── Breathing session state ────────────────────────────────────────────────
static int      selectedExercise  = 0;
static uint8_t  breathPhase       = 0;
static uint32_t phaseStartMs      = 0;
static int      breathCyclesLeft  = 0;

void startBreathing(int exerciseIdx) {
  selectedExercise  = exerciseIdx;
  state             = STATE_BREATHING;
  breathPhase       = 0;
  phaseStartMs      = millis();
  breathCyclesLeft  = EXERCISES[selectedExercise].cycles;

  // Full refresh once at the transition into the exercise (a natural pause
  // point), then only partial refreshes for the duration of the breathing.
  display->EPD_Init();
  display->EPD_Clear();
  display->EPD_DisplayPartBaseImage();
  display->EPD_Init_Partial();
  showBreathingFrame(EXERCISES[selectedExercise], breathPhase, 0);
}

void finishBreathing() {
  state = STATE_PAUSE_COMPLETE;
  display->EPD_Init(); // back to full-refresh mode at this pause point
  showSessionComplete();
  delay(1500);

  state = STATE_IDLE;
  display->EPD_Init();
  showIdle();
}

// ─── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  // Battery fix, part 2: re-assert the latch as the very first firmware
  // instruction in setup(), before Serial, before any delay(), before
  // anything else has a chance to eat into the board's momentary
  // battery-only power window. Defense in depth alongside the constructor.
  board.VBAT_POWER_ON();

  Serial.begin(115200);
  delay(50);
  Serial.println("\n=== Pneuma " FIRMWARE_VERSION " ===");

  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);

  wokeFromUltraSleep = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1);
  resetActivity();

  board.POWER_EPD_ON();
  delay(200);

  custom_lcd_spi_t dispCfg = {};
  dispCfg.cs         = EPD_CS_PIN;
  dispCfg.dc         = EPD_DC_PIN;
  dispCfg.rst        = EPD_RST_PIN;
  dispCfg.busy       = EPD_BUSY_PIN;
  dispCfg.mosi       = EPD_MOSI_PIN;
  dispCfg.scl        = EPD_SCK_PIN;
  dispCfg.spi_host   = EPD_SPI_NUM;
  dispCfg.buffer_len = (EPD_WIDTH * EPD_HEIGHT) / 8;

  display = new epaper_driver_display(EPD_WIDTH, EPD_HEIGHT, dispCfg);
  display->EPD_Init();
  display->EPD_Clear();

  i2c_master_Init();
  delay(50);
  rtcSyncSystemFromChip(); // silently leaves timeReady=false if unset/unknown chip

  batteryInit();

  state = STATE_IDLE;
  showIdle();
}

// ─── Main loop ────────────────────────────────────────────────────────────
void loop() {

  if (state != STATE_BREATHING && millis() - lastActivityMs > ULTRA_SLEEP_MS) {
    enterUltraSleep();
    return;
  }

  if (state != STATE_BREATHING && millis() - lastBatCheckMs > BAT_CHECK_INTERVAL_MS) {
    lastBatCheckMs = millis();
    int pct = readBatteryPercent();
    if (pct >= 0 && pct <= BAT_LOW_THRESHOLD && !batLowWarned) {
      batLowWarned = true;
      display->EPD_Init();
      showBatteryLow(pct);
      delay(2000);
      display->EPD_Init();
      if (state == STATE_IDLE) showIdle();
    } else if (pct > BAT_RECOVER_THRESHOLD) {
      batLowWarned = false;
    }
  }

  // IDLE ─────────────────────────────────────────────────────────────────
  if (state == STATE_IDLE) {
    ButtonEvent a = readButtonEvent(BTN_A);
    if (a == EV_SINGLE || a == EV_LONG) {
      menuCursor = 0;
      state = STATE_MENU;
      display->EPD_Init();
      showMenu(menuCursor);
    }
  }

  // MENU ────────────────────────────────────────────────────────────────
  else if (state == STATE_MENU) {
    ButtonEvent b = readButtonEvent(BTN_B);
    ButtonEvent a = readButtonEvent(BTN_A);

    if (b == EV_SINGLE) {
      menuCursor = (menuCursor + 1) % EXERCISE_COUNT;
      showMenu(menuCursor);
    } else if (a == EV_SINGLE) {
      startBreathing(menuCursor);
    } else if (a == EV_LONG) {
      state = STATE_IDLE;
      display->EPD_Init();
      showIdle();
    }
  }

  // BREATHING ───────────────────────────────────────────────────────────
  else if (state == STATE_BREATHING) {
    const BreathPattern& pattern = EXERCISES[selectedExercise];
    uint32_t elapsed = millis() - phaseStartMs;

    showBreathingFrame(pattern, breathPhase, elapsed);

    if (elapsed >= (uint32_t)pattern.phases[breathPhase].seconds * 1000) {
      breathPhase = (breathPhase + 1) % pattern.phaseCount;
      phaseStartMs = millis();

      if (breathPhase == 0) {
        breathCyclesLeft--;
        if (breathCyclesLeft <= 0) {
          finishBreathing();
          return;
        }
      }
    }

    ButtonEvent a = readButtonEvent(BTN_A);
    if (a == EV_LONG) { // long-press cancels back to idle mid-session
      state = STATE_IDLE;
      display->EPD_Init();
      showIdle();
    }
  }

  delay(30);
}
