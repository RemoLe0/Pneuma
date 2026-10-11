#include "Arduino.h"
#include "../../config.h"
#include "../../globals.h"
#include "sleep.h"
#include "ui.h"

void resetActivity() {
  lastActivityMs = millis();
}

void enterUltraSleep() {
  showUltraSleepScreen();
  delay(120);

  // Keep the battery rail latched through sleep — without this, the board
  // would lose power entirely instead of entering a proper wake-able sleep.
  board.VBAT_POWER_ON();

  uint64_t wakeMask = (1ULL << WAKE_PIN_A) | (1ULL << WAKE_PIN_B);
  esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);

  delay(50);
  esp_deep_sleep_start();
}
