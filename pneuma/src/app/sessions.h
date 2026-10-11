#pragma once

// A Session is a sequence of Blocks. Each Block is one of three kinds:
//
//   BLOCK_BREATH   — phase-paced breathing (inhale/hold/exhale/hold),
//                    optionally with stage markers (see BreathStageMarker)
//   BLOCK_BELLOWS  — fast rhythmic breathing in rounds, with a rest
//                    period between rounds
//   BLOCK_BODYSCAN — a plain duration, attention-guided, no breath pacing
//
// A single-block Session (e.g. "Box") behaves exactly like the old
// single-exercise model. A multi-block Session (e.g. the 12-Minuten-
// Methode) runs its blocks back-to-back with a short named transition
// screen between them.
//
// NOTE: all timings below are reference values from the source material,
// not confirmed against what you actually intend — check before relying
// on them.

enum BlockType { BLOCK_BREATH, BLOCK_BELLOWS, BLOCK_BODYSCAN };

struct BreathPhase {
  int         seconds;
  const char* label;
};

// Marks the cycle index (0-based) at which a new "stage" begins, for
// exercises like Drei-Stufen-Atmung where the rhythm stays constant but
// the user changes hand position partway through. The device shows the
// label only — it does not explain the hand position itself, that's
// assumed physical knowledge the user brings.
struct BreathStageMarker {
  int         cycleIndex;
  const char* label;
};

struct SessionBlock {
  BlockType   type;
  const char* name; // shown briefly on the transition screen when this block starts

  // ── BLOCK_BREATH fields (ignored otherwise) ──────────────────────────
  BreathPhase phases[4];
  int         phaseCount;
  int         cycles;
  const BreathStageMarker* stages;
  int                       stageCount;

  // ── BLOCK_BELLOWS fields (ignored otherwise) ─────────────────────────
  int bellowsBreathSeconds;   // duration of one quick in/out pulse
  int bellowsBreathsPerRound; // e.g. 15
  int bellowsRounds;          // e.g. 3
  int bellowsRestSeconds;     // pause between rounds

  // ── BLOCK_BODYSCAN fields (ignored otherwise) ────────────────────────
  int bodyscanDurationSeconds;
};

struct ExerciseSession {
  const char*         name;
  const SessionBlock* blocks;
  int                 blockCount;
};

extern const ExerciseSession SESSIONS[];
extern const int             SESSION_COUNT;
