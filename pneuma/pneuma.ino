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
#include "src/app/sessions.h"
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

// ─── Session/block runner state ─────────────────────────────────────────
static int      selectedSession = 0;
static int      currentBlock    = 0;

// BLOCK_BREATH sub-state
static uint8_t  breathPhase      = 0;
static uint32_t phaseStartMs     = 0;
static int      breathCycleIndex = 0; // 0-based, counts UP (needed for stage markers)

// BLOCK_BELLOWS sub-state
static int      bellowsRound       = 0;
static int      bellowsBreathIndex = 0;
static uint32_t bellowsPhaseStartMs = 0;
static bool     bellowsResting      = false;
static uint32_t bellowsRestStartMs  = 0;

// BLOCK_BODYSCAN sub-state
static uint32_t bodyscanStartMs      = 0;
static uint32_t bodyscanLastUpdateMs = 0;
#define BODYSCAN_UPDATE_INTERVAL_MS 15000UL

// Pause (applies uniformly across all block types, see handlePauseToggle)
static bool     sessionPaused = false;
static uint32_t pauseStartMs  = 0;

void startBlock() {
  const SessionBlock& block = SESSIONS[selectedSession].blocks[currentBlock];

  display->EPD_Init(); // full refresh at every block transition (natural pause point)
  showBlockIntro(block.name);
  delay(1200);
  display->EPD_Init();
  display->EPD_Clear();
  display->EPD_DisplayPartBaseImage();
  display->EPD_Init_Partial();

  if (block.type == BLOCK_BREATH) {
    breathPhase = 0;
    phaseStartMs = millis();
    breathCycleIndex = 0;
    showBreathingFrame(block, breathPhase, breathCycleIndex, 0);
  }
  else if (block.type == BLOCK_BELLOWS) {
    bellowsRound = 0;
    bellowsBreathIndex = 0;
    bellowsPhaseStartMs = millis();
    bellowsResting = false;
    showBellowsFrame(block, bellowsRound, bellowsBreathIndex, 0);
  }
  else if (block.type == BLOCK_BODYSCAN) {
    bodyscanStartMs = millis();
    bodyscanLastUpdateMs = millis();
    showBodyScan(block, 0);
  }
}

void finishSession() {
  state = STATE_PAUSE_COMPLETE;
  display->EPD_Init(); // back to full-refresh mode at this pause point
  showSessionComplete();
  delay(1500);

  state = STATE_IDLE;
  display->EPD_Init();
  showIdle();
}

void advanceBlock() {
  currentBlock++;
  if (currentBlock >= SESSIONS[selectedSession].blockCount) {
    finishSession();
  } else {
    startBlock();
  }
}

void startSession(int sessionIdx) {
  selectedSession = sessionIdx;
  currentBlock    = 0;
  state           = STATE_BREATHING; // generic "a session is running" state
  sessionPaused   = false;
  startBlock();
}

// Pausing/resuming works the same way regardless of which block type is
// active — the only per-type difference is which timer needs its start
// timestamp shifted forward by the paused duration, so elapsed-time math
// afterwards behaves as if no time passed at all.
void handlePauseToggle() {
  const SessionBlock& block = SESSIONS[selectedSession].blocks[currentBlock];

  if (!sessionPaused) {
    sessionPaused = true;
    pauseStartMs = millis();
    display->EPD_Init(); // pausing is a natural pause point -> full refresh
    showPaused(block.name);
    return;
  }

  uint32_t pausedMs = millis() - pauseStartMs;
  sessionPaused = false;

  if (block.type == BLOCK_BREATH) {
    phaseStartMs += pausedMs;
  } else if (block.type == BLOCK_BELLOWS) {
    if (bellowsResting) bellowsRestStartMs += pausedMs;
    else                bellowsPhaseStartMs += pausedMs;
  } else if (block.type == BLOCK_BODYSCAN) {
    bodyscanStartMs += pausedMs;
    bodyscanLastUpdateMs += pausedMs;
  }

  // Same partial-refresh setup used whenever a block first starts —
  // we just left a full-refresh Paused screen, so this needs redoing
  // regardless of block type (matches startBlock()).
  display->EPD_Init();
  display->EPD_Clear();
  display->EPD_DisplayPartBaseImage();
  display->EPD_Init_Partial();

  if (block.type == BLOCK_BREATH) {
    showBreathingFrame(block, breathPhase, breathCycleIndex, millis() - phaseStartMs);
  } else if (block.type == BLOCK_BELLOWS) {
    if (bellowsResting) showBellowsRest(block, bellowsRound + 1);
    else                showBellowsFrame(block, bellowsRound, bellowsBreathIndex, millis() - bellowsPhaseStartMs);
  } else if (block.type == BLOCK_BODYSCAN) {
    showBodyScan(block, millis() - bodyscanStartMs);
  }
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
      menuCursor = (menuCursor + 1) % SESSION_COUNT;
      showMenu(menuCursor);
    } else if (a == EV_SINGLE) {
      startSession(menuCursor);
    } else if (a == EV_LONG) {
      state = STATE_IDLE;
      display->EPD_Init();
      showIdle();
    }
  }

  // BREATHING = "a session is running" ────────────────────────────────────
  else if (state == STATE_BREATHING) {
    ButtonEvent b = readButtonEvent(BTN_B);
    if (b == EV_SINGLE) {
      handlePauseToggle();
    }

    if (sessionPaused) {
      if (millis() - pauseStartMs > SESSION_PAUSE_TIMEOUT_MS) {
        // Discarded, not "cancelled" via button — no need for the Done
        // screen. Deliberately don't reset lastActivityMs here: it's
        // already been stale for SESSION_PAUSE_TIMEOUT_MS (longer than
        // ULTRA_SLEEP_MS), so the normal idle-sleep check above will
        // send the device to sleep on its own on the next loop — no
        // separate sleep handling needed for this path.
        sessionPaused = false;
        state = STATE_IDLE;
        display->EPD_Init();
        showIdle();
      }
    } else {
      const SessionBlock& block = SESSIONS[selectedSession].blocks[currentBlock];

      if (block.type == BLOCK_BREATH) {
        uint32_t elapsed = millis() - phaseStartMs;
        showBreathingFrame(block, breathPhase, breathCycleIndex, elapsed);

        if (elapsed >= (uint32_t)block.phases[breathPhase].seconds * 1000) {
          breathPhase = (breathPhase + 1) % block.phaseCount;
          phaseStartMs = millis();

          if (breathPhase == 0) {
            breathCycleIndex++;
            if (breathCycleIndex >= block.cycles) {
              advanceBlock();
              return;
            }
          }
        }
      }

      else if (block.type == BLOCK_BELLOWS) {
        if (bellowsResting) {
          uint32_t restElapsed = millis() - bellowsRestStartMs;
          if (restElapsed >= (uint32_t)block.bellowsRestSeconds * 1000) {
            bellowsRound++;
            if (bellowsRound >= block.bellowsRounds) {
              advanceBlock();
              return;
            }
            bellowsResting = false;
            bellowsBreathIndex = 0;
            bellowsPhaseStartMs = millis();
            display->EPD_Init(); // leaving the rest screen -> back to partial mode
            display->EPD_Clear();
            display->EPD_DisplayPartBaseImage();
            display->EPD_Init_Partial();
            showBellowsFrame(block, bellowsRound, bellowsBreathIndex, 0);
          }
        } else {
          uint32_t elapsed = millis() - bellowsPhaseStartMs;
          showBellowsFrame(block, bellowsRound, bellowsBreathIndex, elapsed);

          if (elapsed >= (uint32_t)block.bellowsBreathSeconds * 1000) {
            bellowsBreathIndex++;
            bellowsPhaseStartMs = millis();

            if (bellowsBreathIndex >= block.bellowsBreathsPerRound) {
              bellowsResting = true;
              bellowsRestStartMs = millis();
              display->EPD_Init(); // rest is a natural pause point -> full refresh
              showBellowsRest(block, bellowsRound + 1);
            }
          }
        }
      }

      else if (block.type == BLOCK_BODYSCAN) {
        uint32_t elapsed = millis() - bodyscanStartMs;

        if (millis() - bodyscanLastUpdateMs >= BODYSCAN_UPDATE_INTERVAL_MS) {
          showBodyScan(block, elapsed);
          bodyscanLastUpdateMs = millis();
        }

        if (elapsed >= (uint32_t)block.bodyscanDurationSeconds * 1000) {
          advanceBlock();
          return;
        }
      }
    }

    ButtonEvent a = readButtonEvent(BTN_A);
    if (a == EV_LONG) { // long-press cancels back to idle, paused or not
      sessionPaused = false;
      state = STATE_IDLE;
      display->EPD_Init();
      showIdle();
    }
  }

  delay(30);
}
