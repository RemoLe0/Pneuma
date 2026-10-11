#include "Arduino.h"
#include <string.h>
#include "ui.h"
#include "draw.h"
#include "../../config.h"
#include "../../globals.h"

// TODO: replace with the periodically-updated prompt_today.json content
// once the leitgedanke file pipeline is built. Placeholder for now so the
// idle screen has something meaningful to show.
static const char* LEITGEDANKE = "Notice one thing you usually rush past.";

static void fullRefresh()    { display->EPD_Display(); }
static void partialRefresh() { display->EPD_DisplayPart(); }

static void drawFrame() {
  strokeRoundRect(6, 6, EPD_WIDTH - 12, EPD_HEIGHT - 12, 10, 1, BLACK);
}

// Small, unobtrusive corner hint — deliberately cheap: it's redrawn as
// part of the normal full-buffer redraw every frame, so it costs nothing
// extra in refresh terms. Kept out of the way of the centered breathing
// circle and the bottom progress dots.
static void drawPauseHint(const char* text) {
  drawStr(10, 197, text, 1, BLACK);
}

// ─── Idle ───────────────────────────────────────────────────────────────
void showIdle() {
  clearWhite();
  drawFrame();
  drawStrC(100, 34, "PNEUMA", 2, BLACK);
  drawWrappedText(24, 80, 152, 16, 4, String(LEITGEDANKE), BLACK);
  drawStrC(100, 178, "press to choose", 1, BLACK);
  fullRefresh(); // natural pause point -> full refresh, per display strategy
}

// ─── Session menu ─────────────────────────────────────────────────────────
void showMenu(int cursor) {
  clearWhite();
  drawFrame();
  drawStrC(100, 26, "CHOOSE", 1, BLACK);

  for (int i = 0; i < SESSION_COUNT; i++) {
    int y = 54 + i * 32;
    bool active = (i == cursor);
    if (active) fillRoundRect(20, y - 13, 160, 26, 8, BLACK);
    drawStrC(100, y - 5, SESSIONS[i].name, 1, active ? WHITE : BLACK);
  }

  fullRefresh(); // menu changes are infrequent -> full refresh is fine here
}

// ─── Block transition ─────────────────────────────────────────────────────
// Shown briefly whenever a session moves from one block to the next
// (e.g. Drei-Stufen-Atmung -> Blasebalg -> Body-Scan). A natural pause
// point, so full refresh, same as idle/menu.
void showBlockIntro(const char* name) {
  clearWhite();
  drawFrame();
  drawStrC(100, 100, name, 1, BLACK);
  fullRefresh();
}

// ─── Paused ─────────────────────────────────────────────────────────────
// A natural pause point (no pun intended) -> full refresh, same reasoning
// as idle/menu/block-intro. Shows which block was paused so resuming
// doesn't require remembering where you were.
void showPaused(const char* blockName) {
  clearWhite();
  drawFrame();
  drawStrC(100, 90, "Paused", 2, BLACK);
  drawStrC(100, 120, blockName, 1, BLACK);
  drawPauseHint("B resume");
  fullRefresh();
}

// ─── Breathing (partial refresh only, never mid-breath is skipped) ──────
// Works for any phase count from 1–4, driven entirely by `block` — no
// exercise-specific logic here, so editing patterns in sessions.cpp is
// enough on its own, nothing in ui.cpp needs to change alongside it.
void showBreathingFrame(const SessionBlock& block, int phase, int cycleIndex, uint32_t elapsedMs) {
  const BreathPhase& current = block.phases[phase];
  int totalMs = current.seconds * 1000;
  float t = totalMs > 0 ? (float)elapsedMs / (float)totalMs : 1.0f;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  const int minR = 30, maxR = 70;
  int r;
  String label = current.label;

  bool isHold = (strstr(current.label, "HOLD") != nullptr);
  bool isFirstPhase = (phase == 0);

  if (isHold)            r = maxR;
  else if (isFirstPhase) r = minR + (int)((maxR - minR) * t); // treat first phase as inhale
  else                   r = maxR - (int)((maxR - minR) * t); // treat later non-hold phases as exhale

  // Resolve the active stage label, if this block defines any.
  const char* stageLabel = nullptr;
  for (int i = block.stageCount - 1; i >= 0; i--) {
    if (cycleIndex >= block.stages[i].cycleIndex) {
      stageLabel = block.stages[i].label;
      break;
    }
  }

  clearWhite();
  strokeCircle(100, 100, maxR + 8, 1, BLACK);
  fillCircle(100, 100, r, BLACK);
  drawStrC(100, 176, label.c_str(), 1, BLACK);

  if (stageLabel) {
    drawStrC(100, 30, stageLabel, 1, BLACK);
  }

  drawPauseHint("B pause");

  for (int i = 0; i < block.phaseCount; i++) {
    int x = 100 - (block.phaseCount - 1) * 8 + i * 16;
    if (i == phase) fillCircle(x, 190, 3, BLACK);
    else            strokeCircle(x, 190, 3, 1, BLACK);
  }

  partialRefresh(); // deliberately never a full refresh here — mid-breath
                     // full refreshes would be exactly the kind of visual
                     // disruption this device is built to avoid
}

// ─── Bellows (fast rhythmic pulses within a round) ───────────────────────
void showBellowsFrame(const SessionBlock& block, int round, int breathIndex, uint32_t elapsedMs) {
  int totalMs = block.bellowsBreathSeconds * 1000;
  float t = totalMs > 0 ? (float)elapsedMs / (float)totalMs : 1.0f;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  // Simple pulse: expands in the first half of the pulse, contracts in
  // the second — one full expand/contract per quick breath, no separate
  // inhale/exhale labels needed at this speed.
  const int minR = 40, maxR = 65;
  int r = (t < 0.5f)
    ? minR + (int)((maxR - minR) * (t * 2.0f))
    : maxR - (int)((maxR - minR) * ((t - 0.5f) * 2.0f));

  clearWhite();
  fillCircle(100, 100, r, BLACK);

  char roundBuf[16];
  snprintf(roundBuf, sizeof(roundBuf), "Runde %d/%d", round + 1, block.bellowsRounds);
  drawStrC(100, 30, roundBuf, 1, BLACK);

  char countBuf[16];
  snprintf(countBuf, sizeof(countBuf), "%d/%d", breathIndex + 1, block.bellowsBreathsPerRound);
  drawStrC(100, 176, countBuf, 1, BLACK);

  drawPauseHint("B pause");

  partialRefresh(); // same reasoning as the breathing screen — no full
                     // refresh while actively pulsing through a round
}

void showBellowsRest(const SessionBlock& block, int roundJustFinished) {
  clearWhite();
  drawFrame();
  char buf[32];
  if (roundJustFinished >= block.bellowsRounds) {
    snprintf(buf, sizeof(buf), "Rest");
  } else {
    snprintf(buf, sizeof(buf), "Rest before round %d", roundJustFinished + 1);
  }
  drawStrC(100, 100, buf, 1, BLACK);
  drawPauseHint("B pause");
  fullRefresh(); // the rest itself is a natural pause point
}

// ─── Body-scan (duration-guided, not breath-paced) ───────────────────────
// Deliberately understated: a slow, infrequently-updated progress bar
// rather than an animated breathing circle. The content here is
// attention-guided, not breath-synced, and matches the "boring by
// design" philosophy better with a near-static screen than with motion.
void showBodyScan(const SessionBlock& block, uint32_t elapsedMs) {
  clearWhite();
  drawFrame();
  drawStrC(100, 60, "Body-Scan", 1, BLACK);

  int totalMs = block.bodyscanDurationSeconds * 1000;
  float t = totalMs > 0 ? (float)elapsedMs / (float)totalMs : 0.0f;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  int barW = 140;
  int fillW = (int)(barW * t);
  strokeRect(30, 110, barW, 10, 1, BLACK);
  if (fillW > 0) fillRect(30, 110, fillW, 10, BLACK);

  drawPauseHint("B pause");

  partialRefresh(); // called infrequently by the caller (every ~15s), so
                     // this stays true to the "slow by design" intent
                     // without leaving the screen looking frozen/broken
}

// ─── Pause points ─────────────────────────────────────────────────────────
void showSessionComplete() {
  clearWhite();
  drawFrame();
  drawStrC(100, 90, "Done.", 2, BLACK);
  fullRefresh();
}

void showBatteryLow(int pct) {
  clearWhite();
  drawFrame();
  drawStrC(100, 80, "Battery low", 1, BLACK);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  drawStrC(100, 110, buf, 2, BLACK);
  fullRefresh();
}

void showUltraSleepScreen() {
  clearWhite();
  drawFrame();
  drawStrC(100, 100, "Sleeping.", 1, BLACK);
  fullRefresh();
}
