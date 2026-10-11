#include "Arduino.h"
#include "../../config.h"
#include "../../globals.h"
#include "buttons.h"

extern void resetActivity();

// Same debounce/long-press/double-press logic applies uniformly to every
// button pin passed in — no special-casing per pin. Works for 1, 2, or
// more buttons without changes here; button count/mapping lives in config.h.
bool isDown(int pin) { return digitalRead(pin) == LOW; }

ButtonEvent readButtonEvent(int pin) {
  if (!isDown(pin)) return EV_NONE;
  delay(6);
  if (!isDown(pin)) return EV_NONE;

  uint32_t t0 = millis();
  while (isDown(pin)) {
    if (millis() - t0 > BTN_LONG_MS) {
      while (isDown(pin)) delay(3);
      resetActivity();
      return EV_LONG;
    }
    delay(3);
  }

  uint32_t rel = millis();
  while (millis() - rel < DOUBLE_MS) {
    if (isDown(pin)) {
      delay(10);
      if (isDown(pin)) {
        while (isDown(pin)) delay(3);
        resetActivity();
        return EV_DOUBLE;
      }
    }
    delay(3);
  }

  resetActivity();
  return EV_SINGLE;
}
