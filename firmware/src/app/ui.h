#pragma once
#include <Arduino.h>
#include "exercises.h"

void showIdle();
void showMenu(int cursor);
void showBreathingFrame(const BreathPattern& pattern, int phase, uint32_t elapsedMs);
void showSessionComplete();
void showBatteryLow(int pct);
void showUltraSleepScreen();
