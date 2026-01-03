#pragma once

#include <stdbool.h>
#include <stdint.h>

bool flash_erase_page(uint16_t page);
bool flash_write_page(uint16_t page, uint8_t* data, uint16_t data_size);
uint32_t flash_copy_page(uint16_t page, uint8_t* data, uint16_t data_size);