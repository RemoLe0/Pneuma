#include "sessions.h"

// ── 12-Minuten-Methode (Christoph Glaser) ──────────────────────────────
// Reference timings from the source material. Confirm before relying on
// them, especially the Body-Scan duration (source says "1 to 2 minutes",
// 90s picked as a midpoint placeholder).

static const BreathStageMarker DREI_STUFEN_STAGES[] = {
  { 0,  "Stufe 1" },
  { 8,  "Stufe 2" },
  { 16, "Stufe 3" },
};

static const SessionBlock TWELVE_MIN_BLOCKS[] = {
  {
    BLOCK_BREATH, "Drei-Stufen-Atmung",
    { {4, "IN"}, {4, "HOLD"}, {6, "OUT"}, {2, "HOLD"} }, 4, 22,
    DREI_STUFEN_STAGES, 3,
    0, 0, 0, 0,   // bellows fields unused
    0,            // bodyscan field unused
  },
  {
    BLOCK_BELLOWS, "Blasebalg",
    {}, 0, 0, nullptr, 0,   // breath fields unused
    1, 15, 3, 15,           // ~1s/breath, 15 breaths/round, 3 rounds, 15s rest
    0,                      // bodyscan field unused
  },
  {
    BLOCK_BODYSCAN, "Body-Scan",
    {}, 0, 0, nullptr, 0,   // breath fields unused
    0, 0, 0, 0,             // bellows fields unused
    90,                     // seconds
  },
};

// ── Quick single-block sessions ─────────────────────────────────────────
static const SessionBlock BOX_BLOCKS[] = {
  { BLOCK_BREATH, "Box",
    { {4, "IN"}, {4, "HOLD"}, {4, "OUT"}, {4, "HOLD"} }, 4, 4,
    nullptr, 0,  0, 0, 0, 0,  0 },
};

static const SessionBlock FOUR_SEVEN_EIGHT_BLOCKS[] = {
  { BLOCK_BREATH, "4-7-8",
    { {4, "IN"}, {7, "HOLD"}, {8, "OUT"}, {0, ""} }, 3, 4,
    nullptr, 0,  0, 0, 0, 0,  0 },
};

static const SessionBlock COHERENT_BLOCKS[] = {
  { BLOCK_BREATH, "Coherent",
    { {5, "IN"}, {5, "OUT"}, {0, ""}, {0, ""} }, 2, 6,
    nullptr, 0,  0, 0, 0, 0,  0 },
};

const ExerciseSession SESSIONS[] = {
  { "12-Minuten-Methode", TWELVE_MIN_BLOCKS,      3 },
  { "Box",                BOX_BLOCKS,             1 },
  { "4-7-8",              FOUR_SEVEN_EIGHT_BLOCKS,1 },
  { "Coherent",           COHERENT_BLOCKS,        1 },
};

const int SESSION_COUNT = sizeof(SESSIONS) / sizeof(SESSIONS[0]);
