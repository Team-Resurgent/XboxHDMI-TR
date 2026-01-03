#include "flash.h"

#include "defines.h"
#include "smbus_i2c.h"
#include "stm32.h"
#include "crc32.h"

bool flash_erase_page(uint16_t page)
{
    uint32_t flash_addr = FLASH_START_ADDRESS + (page * FLASH_PAGE_SIZE);

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef eraseInitStruct;
    eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.PageAddress = flash_addr;
    eraseInitStruct.NbPages = 1;

    uint32_t pageError = 0;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInitStruct, &pageError);

    HAL_FLASH_Lock();

    return status == HAL_OK;
}

bool flash_write_page(uint16_t page, uint8_t* data, uint16_t data_size)
{
    uint32_t flash_addr = FLASH_START_ADDRESS + (page * FLASH_PAGE_SIZE);

    HAL_FLASH_Unlock();

    HAL_StatusTypeDef status = HAL_OK;
    for (uint32_t i = 0; i < data_size; i += 2)
    {
        uint16_t half_word = data[i] | (data[i + 1] << 8);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_addr + i, half_word);
        if (status != HAL_OK)
        {
            break;
        }
    }

    HAL_FLASH_Lock();

    return status == HAL_OK;
}

uint32_t flash_copy_page(uint16_t page, uint8_t* data, uint16_t data_size)
{
    uint32_t flash_addr = FLASH_START_ADDRESS + (page * FLASH_PAGE_SIZE);
    return crc32_copy(flash_addr, data, data_size);
}