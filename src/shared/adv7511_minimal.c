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

// New stuff
void update_avi_infoframe(const bool widescreen) {
    // [6] Start AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b01000000);
    // [6:5] Infoframe output format to YCbCr4:4:4
    adv7511_update_register(0x55, 0b01100000, 0b01000000);
    // [5:4] Set aspect ratio
    // [3:0] Active format aspect ratio, same as aspect ratio
    adv7511_write_register(0x56, widescreen ? 0b00101000 : 0b00011000);
    // [6] End AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b00000000);
}

void init_adv_audio() {
    // [19:0] Set 48kHz Audio clock CHECK (N Value)
    adv7511_write_register(0x01, 0x00);
    adv7511_write_register(0x02, 0x18);
    adv7511_write_register(0x03, 0x00);

    // [6:4] Set SPDIF audio source
    adv7511_update_register(0x0A, 0b01110000, 0b00010000);

    // [7] SPDIF enable
    adv7511_update_register(0x0B, 0b10000000, 0b10000000);
}
