#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "../shared/defines.h"
#include "../shared/led.h"
#include <string.h>
#include "../shared/debug.h"
#include "../shared/error_handler.h"

#include <stdbool.h>

static bool can_launch_application(void);
static void jump_to_application(void);
static void enter_bootloader_mode(void);
static void flash_erase_page(uint32_t address);
static HAL_StatusTypeDef flash_write(uint32_t address, uint8_t *data, uint32_t len);
static uint32_t calculate_checksum(uint32_t start, uint32_t length);

extern void SystemClock_Config(void);
volatile uint8_t bootloader_running = 0;

int main(void) 
{    
    bootloader_running = 1;

    HAL_Init();    
    SystemClock_Config();

    debug_init();
    debug_log("Entering Bootloader...\r\n");

    uint32_t flag_value = *BOOTLOADER_FLAG_ADDRESS;
    if (flag_value != BOOTLOADER_MAGIC_VALUE && can_launch_application())
    {
        jump_to_application();
    }
    *BOOTLOADER_FLAG_ADDRESS = 0;
    enter_bootloader_mode();
}

static bool can_launch_application(void)
{
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t stack_pointer = app_vector_table[0];
    uint32_t app_entry = app_vector_table[1];

    if (stack_pointer < RAM_START_ADDRESS || stack_pointer > (RAM_START_ADDRESS + RAM_TOTAL_SIZE)) {
        return false;
    }

    if (app_entry < FLASH_START_ADDRESS || app_entry > (FLASH_START_ADDRESS + FLASH_TOTAL_SIZE)) {
        return false;
    }
    
    if (stack_pointer == 0xFFFFFFFF && app_entry == 0xFFFFFFFF) {
        return false;
    }

    return true;
}

static void jump_to_application(void) 
{
    debug_log("Launching Application...\r\n");

    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t stack_pointer = app_vector_table[0];
    uint32_t app_entry = app_vector_table[1];

    __disable_irq();

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    bootloader_running = 0;

    asm volatile (
        "mov sp,%0 ; blx %1"
        :: "r" (stack_pointer), "r" (app_entry)
    );

    while (1);
}

static void enter_bootloader_mode(void) 
{
    init_led();

    debug_log("Waiting for update...\r\n");

    static uint32_t last_blink = 0;
    static bool led_state = false;
    
    while(1) {
        if ((HAL_GetTick() - last_blink) > 100) {
            led_state = !led_state;
            set_led_1(led_state); 
            set_led_2(!led_state); 
            last_blink = HAL_GetTick();
        }
        HAL_Delay(10);
    }
}

static void flash_erase_page(uint32_t address) {
    HAL_FLASH_Unlock();
    
    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = address;
    erase.NbPages = 1;
    
    uint32_t page_error = 0;
    HAL_FLASHEx_Erase(&erase, &page_error);
    
    HAL_FLASH_Lock();
}

static HAL_StatusTypeDef flash_write(uint32_t address, uint8_t *data, uint32_t len) {
    HAL_FLASH_Unlock();
    
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t *data32 = (uint32_t *)data;
    uint32_t words = (len + 3) / 4; // Round up to word count
    
    for (uint32_t i = 0; i < words; i++) {
        uint32_t word = 0xFFFFFFFF;
        if (i * 4 < len) {
            word = data32[i];
        }
        
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + (i * 4), word);
        if (status != HAL_OK) {
            break;
        }
    }
    
    HAL_FLASH_Lock();
    return status;
}

static uint32_t calculate_checksum(uint32_t start, uint32_t length) {
    uint32_t checksum = 0;
    uint32_t *data = (uint32_t *)start;
    uint32_t words = length / 4;
    
    for (uint32_t i = 0; i < words; i++) {
        checksum += data[i];
    }
    
    return checksum;
}

