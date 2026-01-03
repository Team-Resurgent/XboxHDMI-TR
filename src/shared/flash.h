#pragma once

#include <stdbool.h>
#include <stdint.h>

static bool flash_erase_page(uint16_t page);
static bool flash_write_page(uint16_t page, uint8_t* data, uint16_t data_size);
static void flash_read_page(uint16_t page, uint8_t* data, uint16_t data_size);
static uint32_t flash_crc32_page(uint16_t page);