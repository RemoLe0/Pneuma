#pragma once
#include <Arduino.h>
#include "sessions.h"

void showIdle();
void showMenu(int cursor);
void showBlockIntro(const char* name);
void showPaused(const char* blockName);
void showBreathingFrame(const SessionBlock& block, int phase, int cycleIndex, uint32_t elapsedMs);
void showBellowsFrame(const SessionBlock& block, int round, int breathIndex, uint32_t elapsedMs);
void showBellowsRest(const SessionBlock& block, int roundJustFinished);
void showBodyScan(const SessionBlock& block, uint32_t elapsedMs);
void showSessionComplete();
void showBatteryLow(int pct);
void showUltraSleepScreen();
