// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#include "adv7511_minimal.h"
#include "../shared/adv7511_i2c.h"

uint8_t adv7511_read_register(const uint8_t address) {
    uint8_t data = 0;
    I2C_HandleTypeDef* hi2c = adv7511_i2c_instance();
    HAL_I2C_Mem_Read(hi2c, ADV7511_MAIN_I2C_ADDR, address, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
    return data;
}

void adv7511_write_register(const uint8_t address, uint8_t value) {
    I2C_HandleTypeDef* hi2c = adv7511_i2c_instance();
    HAL_I2C_Mem_Write(hi2c, ADV7511_MAIN_I2C_ADDR, address, I2C_MEMADD_SIZE_8BIT, &value, 1, HAL_MAX_DELAY);
}

void adv7511_update_register(const uint8_t address, const uint8_t mask, uint8_t new_value) {
    const uint8_t current = adv7511_read_register(address);
    uint8_t updated = (current & ~mask) | (new_value & mask);
    adv7511_write_register(address, updated);
}

void adv7511_struct_init(adv7511 *encoder) {
    encoder->hot_plug_detect = 0;
    encoder->monitor_sense = 0;
    encoder->interrupt = 0;
    encoder->vic = 0;
}

void adv7511_power_up(adv7511 *encoder) {
    // Power up the encoder
    adv7511_write_register(0x41, 0x10); // Power up
    HAL_Delay(20);

    // Reset
    adv7511_write_register(0x41, 0x00);
    HAL_Delay(20);
    adv7511_write_register(0x41, 0x10);
    HAL_Delay(20);
}
