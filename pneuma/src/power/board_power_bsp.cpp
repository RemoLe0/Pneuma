#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "board_power_bsp.h"

board_power_bsp_t::board_power_bsp_t(uint8_t _epd_power_pin, uint8_t _vbat_power_pin) :
    epd_power_pin(_epd_power_pin),
    vbat_power_pin(_vbat_power_pin) {

    gpio_config_t gpio_conf = {};
        gpio_conf.intr_type = GPIO_INTR_DISABLE;
        gpio_conf.mode = GPIO_MODE_OUTPUT;
        gpio_conf.pin_bit_mask = (0x1ULL << epd_power_pin) | (0x1ULL << vbat_power_pin);
        gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    /* ── Battery fix ────────────────────────────────────────────────────
     * This constructor runs during global static initialization, i.e.
     * BEFORE setup() executes. The board only supplies momentary power
     * from the battery when the physical PWR button is pressed; if
     * firmware doesn't drive VBAT_PWR_PIN HIGH before that momentary
     * window closes, the board loses power mid-boot. Running on USB
     * masked this, because USB power bridges the gap regardless of the
     * pin state — which is why the symptom only showed up on battery-only
     * cold boots. Latching here, at the earliest point code can run at
     * all, gives the firmware the best possible chance of winning that
     * race. setup() also re-asserts this as its very first instruction,
     * for defense in depth. */
    gpio_set_level((gpio_num_t)vbat_power_pin, 1);

    /* EPD stays off until explicitly requested (active LOW -> 1 = off). */
    gpio_set_level((gpio_num_t)epd_power_pin, 1);
}

board_power_bsp_t::~board_power_bsp_t() {

}

void board_power_bsp_t::POWER_EPD_ON() {
    gpio_set_level((gpio_num_t)epd_power_pin, 0);
}

void board_power_bsp_t::POWER_EPD_OFF() {
    gpio_set_level((gpio_num_t)epd_power_pin, 1);
}

void board_power_bsp_t::VBAT_POWER_ON() {
    gpio_set_level((gpio_num_t)vbat_power_pin, 1);
}

void board_power_bsp_t::VBAT_POWER_OFF() {
    gpio_set_level((gpio_num_t)vbat_power_pin, 0);
}
