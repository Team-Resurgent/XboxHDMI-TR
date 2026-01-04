#pragma once

#include <stdbool.h>
#include <stdint.h>

uint32_t crc32_calc(uint32_t start_addr, uint32_t length);
uint32_t crc32_copy(uint32_t start_addr, uint8_t* data, uint16_t data_size);
