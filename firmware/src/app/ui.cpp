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

// ─── Idle ───────────────────────────────────────────────────────────────
void showIdle() {
  clearWhite();
  drawFrame();
  drawStrC(100, 34, "PNEUMA", 2, BLACK);
  drawWrappedText(24, 80, 152, 16, 4, String(LEITGEDANKE), BLACK);
  drawStrC(100, 178, "press to choose", 1, BLACK);
  fullRefresh(); // natural pause point -> full refresh, per display strategy
}

// ─── Exercise menu ────────────────────────────────────────────────────────
void showMenu(int cursor) {
  clearWhite();
  drawFrame();
  drawStrC(100, 30, "CHOOSE", 1, BLACK);

  for (int i = 0; i < EXERCISE_COUNT; i++) {
    int y = 60 + i * 34;
    bool active = (i == cursor);
    if (active) fillRoundRect(30, y - 14, 140, 28, 8, BLACK);
    drawStrC(100, y - 6, EXERCISES[i].name, 1, active ? WHITE : BLACK);
  }

  fullRefresh(); // menu changes are infrequent -> full refresh is fine here
}

// ─── Breathing (partial refresh only, never mid-breath is skipped) ──────
// Works for any phase count from 1–4, driven entirely by `pattern` — no
// exercise-specific logic here, so adding/editing patterns in exercises.cpp
// is enough on its own, nothing in ui.cpp needs to change alongside it.
void showBreathingFrame(const BreathPattern& pattern, int phase, uint32_t elapsedMs) {
  const BreathPhase& current = pattern.phases[phase];
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

  clearWhite();
  strokeCircle(100, 100, maxR + 8, 1, BLACK);
  fillCircle(100, 100, r, BLACK);
  drawStrC(100, 176, label.c_str(), 1, BLACK);

  // small cycle/phase progress dots along the bottom
  for (int i = 0; i < pattern.phaseCount; i++) {
    int x = 100 - (pattern.phaseCount - 1) * 8 + i * 16;
    if (i == phase) fillCircle(x, 190, 3, BLACK);
    else            strokeCircle(x, 190, 3, 1, BLACK);
  }

  partialRefresh(); // deliberately never a full refresh here — mid-breath
                     // full refreshes would be exactly the kind of visual
                     // disruption this device is built to avoid
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
