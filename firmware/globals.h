#pragma once
#include "types.h"
#include "src/power/board_power_bsp.h"
#include "src/display/epaper_driver_bsp.h"

extern AppState  state;

extern uint32_t  lastActivityMs;
extern bool      wokeFromUltraSleep;

extern uint32_t  lastBatCheckMs;
extern bool      batLowWarned;

extern bool      timeReady;

extern board_power_bsp_t      board;
extern epaper_driver_display* display;
