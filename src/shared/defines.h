#pragma once

// ============================================================================
// BOOTLOADER CONFIGURATION
// ============================================================================

// Bootloader magic flag address (must be in RAM, survives reset)
#define BOOTLOADER_MAGIC_VALUE      0xDEADBEEF
#define BOOTLOADER_FLAG_ADDRESS    ((volatile uint32_t*)0x20000000)

// ============================================================================
// MEMORY LAYOUT (16KB bootloader, 48KB application)
// ============================================================================

// Flash constants
#define FLASH_START_ADDRESS     0x08000000
#define FLASH_TOTAL_SIZE        0x10000    // 64KB for STM32F030C8T6

// Bootloader and application addresses (16KB bootloader)
#define BOOTLOADER_SIZE         0x4000  // 16KB
#define APP_START_ADDRESS       0x08004000

// ============================================================================
// COMPILE-TIME VALIDATION
// ============================================================================

#if (BOOTLOADER_SIZE > FLASH_TOTAL_SIZE)
    #error "Bootloader size exceeds total flash size!"
#endif

#if (APP_START_ADDRESS < FLASH_START_ADDRESS) || (APP_START_ADDRESS > (FLASH_START_ADDRESS + FLASH_TOTAL_SIZE))
    #error "Application start address is invalid!"
#endif
