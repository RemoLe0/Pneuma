#include "exercises.h"

const BreathPattern EXERCISES[] = {
  {
    "Box",
    { {4, "IN"}, {4, "HOLD"}, {4, "OUT"}, {4, "HOLD"} },
    4,   // phaseCount
    4,   // cycles
  },
  {
    "4-7-8",
    { {4, "IN"}, {7, "HOLD"}, {8, "OUT"}, {0, ""} },
    3,   // phaseCount
    4,   // cycles
  },
  {
    "Coherent",
    { {5, "IN"}, {5, "OUT"}, {0, ""}, {0, ""} },
    2,   // phaseCount
    6,   // cycles
  },
};

const int EXERCISE_COUNT = sizeof(EXERCISES) / sizeof(EXERCISES[0]);
