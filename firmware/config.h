#ifndef CONFIG_H
#define CONFIG_H

/* ── Buses ─────────────────────────────────────────────────────────────── */
#define EPD_SPI_NUM        SPI2_HOST
#define ESP32_I2C_DEV_NUM  I2C_NUM_0

/* ── Display ───────────────────────────────────────────────────────────── */
#define EPD_WIDTH  200
#define EPD_HEIGHT 200

/* EPD SPI pins */
#define EPD_DC_PIN    GPIO_NUM_10
#define EPD_CS_PIN    GPIO_NUM_11
#define EPD_SCK_PIN   GPIO_NUM_12
#define EPD_MOSI_PIN  GPIO_NUM_13
#define EPD_RST_PIN   GPIO_NUM_9
#define EPD_BUSY_PIN  GPIO_NUM_8

/* ── Power control pins ────────────────────────────────────────────────── */
#define EPD_PWR_PIN     GPIO_NUM_6   // active LOW (confirmed during bring-up)
#define VBAT_PWR_PIN    GPIO_NUM_17  // active HIGH — ALSO the power latch pin
#define PWR_HOLD_PIN    17           // same physical pin as VBAT_PWR_PIN.
                                      // The board only supplies momentary power
                                      // on a button press; firmware must drive
                                      // this HIGH immediately to stay powered.
                                      // See board_power_bsp.cpp constructor.
#define BAT_ADC_PIN     4            // ADC1_CHANNEL_3 on ESP32-S3

/* ── Buttons ───────────────────────────────────────────────────────────── *
 * BTN_A: confirmed physical PWR button.
 * BTN_B: PLACEHOLDER — adjust to your actual second-button wiring.
 *        Do not leave this on GPIO0 (BOOT strapping pin) in the final
 *        hardware revision unless you've confirmed it doesn't interfere
 *        with reset/flashing behavior on your board.
 * ------------------------------------------------------------------------ */
#define BTN_A GPIO_NUM_18
#define BTN_B GPIO_NUM_0

/* Deep-sleep wake pins (must be RTC-capable GPIOs on ESP32-S3) */
#define WAKE_PIN_A GPIO_NUM_18
#define WAKE_PIN_B GPIO_NUM_0

/* ── I2C / RTC ─────────────────────────────────────────────────────────── */
#define ESP32_I2C_SDA_PIN GPIO_NUM_47
#define ESP32_I2C_SCL_PIN GPIO_NUM_48

/* NOTE: register map in rtc.h assumes a PCF85063-compatible chip.
 * Verify against the actual RTC IC on your board before relying on it —
 * different RTC chips use different register layouts. */
#define I2C_RTC_DEV_Address 0x51

/* ── Button timing (ms) ────────────────────────────────────────────────── */
#define BTN_LONG_MS   600
#define DOUBLE_MS     200

/* ── Sleep / activity ──────────────────────────────────────────────────── */
#define ULTRA_SLEEP_MS  600000UL

/* ── Battery ───────────────────────────────────────────────────────────── */
#define BAT_CHECK_INTERVAL_MS  30000
#define BAT_LOW_THRESHOLD      15
#define BAT_RECOVER_THRESHOLD  20

/* Breathing patterns themselves now live in src/app/exercises.cpp, one
 * entry per selectable exercise (name, phases, cycle count) — see that
 * file to add, remove, or correct exercises. Nothing here anymore. */

#define FIRMWARE_VERSION  "v0.2"

#endif // CONFIG_H
