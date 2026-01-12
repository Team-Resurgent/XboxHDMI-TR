// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#ifndef __ADV7511_MINIMAL_H__
#define __ADV7511_MINIMAL_H__

#include "stm32.h"

#define ADV7511_VIC_CHANGED         0x80
#define ADV7511_VIC_CHANGED_CLEAR   0x7F

/* Hardware defined default addresses for I2C register maps */
#define ADV7511_MAIN_I2C_ADDR           0x72 //0x72>>1

typedef struct
{
    uint8_t hot_plug_detect;
    uint8_t monitor_sense;
    uint8_t interrupt;
    uint8_t vic;
} adv7511;

void adv7511_power_up(adv7511 *encoder);
void adv7511_update_register(const uint8_t address, const uint8_t mask, uint8_t new_value);
uint8_t adv7511_read_register(const uint8_t address);
void adv7511_write_register(const uint8_t address, uint8_t value);
void adv7511_struct_init(adv7511 *encoder);

#endif // __ADV7511_MINIMAL_H__
