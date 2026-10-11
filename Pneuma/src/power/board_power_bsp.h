#ifndef BOARD_POWER_BSP_H
#define BOARD_POWER_BSP_H

#include <stdint.h>

class board_power_bsp_t
{
private:
    const uint8_t epd_power_pin;
    const uint8_t vbat_power_pin;

public:
    board_power_bsp_t(uint8_t _epd_power_pin, uint8_t _vbat_power_pin);
    ~board_power_bsp_t();

    void POWER_EPD_ON();
    void POWER_EPD_OFF();
    void VBAT_POWER_ON();
    void VBAT_POWER_OFF();
};

#endif
