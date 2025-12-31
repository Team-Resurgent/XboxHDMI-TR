#include "bootloader.h"
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

typedef void (*app_func_t)(void);

extern void SystemClock_Config(void);

int main(void) {

    if (*BOOTLOADER_FLAG_ADDRESS == BOOTLOADER_MAGIC_VALUE) {
        *BOOTLOADER_FLAG_ADDRESS = 0; 
        enter_bootloader_mode();
    } else if (check_application_valid()) {
        jump_to_application();
    } 
    
    enter_bootloader_mode();
}

static uint32_t check_application_valid(void) {
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

static void jump_to_application(void) {
    uint32_t *app_stack = (uint32_t *)APP_START_ADDRESS;
    uint32_t *app_reset = (uint32_t *)(APP_START_ADDRESS + 4);
    
    // Disable interrupts
    __disable_irq();
    
    // Reset SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    
    // STM32F0 (Cortex-M0) does not have VTOR register.
    // We need to copy the application's vector table to SRAM and remap SRAM to 0x00000000.
    
    // Enable SYSCFG clock (required for memory remapping)
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    
    // Copy vector table from application flash to start of SRAM
    // After remapping, SRAM at 0x20000000 will be accessible at 0x00000000
    // So we copy the vector table to 0x20000000, and it will appear at 0x00000000 after remap
    const uint32_t VECTOR_TABLE_SIZE = 48;  // Number of vectors in Cortex-M0 vector table (48 * 4 = 192 bytes)
    const uint32_t SRAM_VT_ADDR = 0x20000000;   // Start of SRAM (SRAM_BASE is already defined in CMSIS)
    
    uint32_t *app_vectors = (uint32_t *)APP_START_ADDRESS;
    uint32_t *sram_vectors = (uint32_t *)SRAM_VT_ADDR;
    
    // Copy vector table from application flash to SRAM
    for (uint32_t i = 0; i < VECTOR_TABLE_SIZE; i++) {
        sram_vectors[i] = app_vectors[i];
    }
    
    // Remap SRAM to 0x00000000 using SYSCFG_CFGR1
    // MEM_MODE bits [1:0]: 00=Flash, 01=System Flash, 10=SRAM, 11=Reserved
    // Set to 10 (SRAM remapped) = 0x02
    SYSCFG->CFGR1 &= ~(0x03);  // Clear bits [1:0]
    SYSCFG->CFGR1 |= 0x02;     // Set to SRAM remapped (10 in binary)
    
    // Memory barrier to ensure remapping is complete
    __DSB();
    __ISB();
    
    // Set stack pointer
    __set_MSP(app_stack[0]);
    
    // Jump to application reset handler
    app_func_t app_main = (app_func_t)app_reset[0];
    app_main();
}

static void enter_bootloader_mode(void) {

    HAL_Init();
    SystemClock_Config();

    // Initialize hardware only when entering bootloader mode
    init_led();
    debug_init();
    debug_log("Bootloader mode entered\r\n");
    
    // Blink LED to indicate bootloader mode
    set_led_1(true);
    HAL_Delay(100);
    set_led_1(false);

    // Initialize I2C for firmware updates
    // TODO: Implement I2C bootloader protocol
    static uint32_t last_blink = 0;
    static bool led_state = false;
    
    while(1) {
        // Blink LED to show we're in bootloader (2 times per second = 250ms toggle)
        if ((HAL_GetTick() - last_blink) > 250) {
            led_state = !led_state;
            set_led_2(led_state); // Toggle LED
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

