#pragma once

// Each pattern has up to 4 phases (some techniques use fewer, e.g. Coherent
// Breathing has no hold at all). phaseCount tells the runner how many of
// the entries in `phases` are actually used.
//
// NOTE: these are common reference timings, not confirmed against your
// specific intended exercises — check and adjust before relying on them.

struct BreathPhase {
  int         seconds;
  const char* label;
};

struct BreathPattern {
  const char* name;
  BreathPhase phases[4];
  int         phaseCount;
  int         cycles;
};

extern const BreathPattern EXERCISES[];
extern const int           EXERCISE_COUNT;
