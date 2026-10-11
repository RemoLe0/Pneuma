#pragma once

typedef enum {
  STATE_IDLE,
  STATE_MENU,
  STATE_BREATHING,
  STATE_PAUSE_COMPLETE,
  STATE_BATTERY_LOW,
} AppState;

enum ButtonEvent { EV_NONE, EV_SINGLE, EV_LONG, EV_DOUBLE };
