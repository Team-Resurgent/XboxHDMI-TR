#include "crc32.h"

#include <stdbool.h>
#include <stdint.h>

static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void _crc32_init(void)
{
    uint32_t crc;
    for (uint32_t i = 0; i < 256; i++)
    {
        crc = i;
        for (uint32_t j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

static uint32_t crc32_calc(uint32_t start_addr, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *flash_ptr = (const uint8_t *)start_addr;

    if (!crc32_table_initialized)
    {
        _crc32_init();
    }

    for (uint32_t i = 0; i < length; i++)
    {
        crc = (crc >> 8) ^ crc32_table[(crc ^ flash_ptr[i]) & 0xFF];
    }

    return crc ^ 0xFFFFFFFF;
}