#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "../shared/defines.h"
#include "../shared/led.h"
#include <string.h>
#include "../shared/debug.h"
#include "../shared/error_handler.h"

static uint32_t check_application_valid(void);
static void jump_to_application(void);
static void enter_bootloader_mode(void);
static void flash_erase_page(uint32_t address);
static HAL_StatusTypeDef flash_write(uint32_t address, uint8_t *data, uint32_t len);
static uint32_t calculate_checksum(uint32_t start, uint32_t length);

extern void SystemClock_Config(void);

int main(void) 
{    
    // Check bootloader flag first (before any HAL init)
    // if (*BOOTLOADER_FLAG_ADDRESS == BOOTLOADER_MAGIC_VALUE) {
    //     *BOOTLOADER_FLAG_ADDRESS = 0;  // Clear flag
    //     enter_bootloader_mode();
    // } 
    // // Check if application is valid and jump to it immediately
    // else if (check_application_valid()) {

    //     jump_to_application();
    // } 
    // // Otherwise enter bootloader mode
    // else {
    //     // Initialize HAL for bootloader mode

    //     enter_bootloader_mode();
    // }
    
    jump_to_application();
    while(1);
}

static uint32_t check_application_valid(void) 
{
    uint32_t *app_stack = (uint32_t *)APP_START_ADDRESS;
    uint32_t *app_reset = (uint32_t *)(APP_START_ADDRESS + 4);
    
    // Check stack pointer is in valid RAM range (0x20000000 - 0x20002000 for STM32F030C8)
    if (app_stack[0] < 0x20000000 || app_stack[0] > 0x20002000) {
        return 0;
    }
    
    // Check reset handler is in valid flash range
    if (app_reset[0] < FLASH_START_ADDRESS || app_reset[0] > (FLASH_START_ADDRESS + FLASH_TOTAL_SIZE)) {
        return 0;
    }
    
    // Check if it's not all 0xFF (erased flash)
    if (app_stack[0] == 0xFFFFFFFF && app_reset[0] == 0xFFFFFFFF) {
        return 0;
    }
    
    return 1;
}

static void jump_to_application(void) 
{
    // Get application stack pointer from vector table
    // Use volatile to ensure we read from flash, not cache
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_stack = app_vector_table[0];
    uint32_t pc = app_vector_table[1];

    asm volatile (
        "mov sp,%0 ; blx %1"
        :: "r" (app_stack), "r" (pc));

    while(1) {
        // // Flash both LEDs rapidly - slow enough to see
        // set_led_1(true);
        // set_led_2(true);
        // HAL_Delay(200);  // 200ms on
        // set_led_1(false);
        // set_led_2(false);
        // HAL_Delay(200);  // 200ms off
    }
}

static void enter_bootloader_mode(void) 
{
    HAL_Init();
    SystemClock_Config();
    init_led();
    debug_init();
    debug_log("Bootloader mode entered\r\n");

    static uint32_t last_blink = 0;
    static bool led_state = false;
    
    while(1) {
        if ((HAL_GetTick() - last_blink) > 250) {
            led_state = !led_state;
            set_led_1(led_state); 
            set_led_2(!led_state); 
            last_blink = HAL_GetTick();
        }
        HAL_Delay(100);
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

