#pragma once

// ============================================================================
// BOOTLOADER CONFIGURATION
// ============================================================================

#define RAM_START_ADDRESS       0x20000000
#define RAM_TOTAL_SIZE          0x2000
#define FLASH_START_ADDRESS     0x08000000
#define FLASH_TOTAL_SIZE        0x10000

#define BOOTLOADER_MAGIC_VALUE      0xDEADBEEF
#define BOOTLOADER_FLAG_ADDRESS    ((volatile uint32_t*)(RAM_START_ADDRESS + 0xF0))

// Bootloader and application addresses (16KB bootloader)
#define BOOTLOADER_SIZE         0x4000  // 16KB
#define APP_START_ADDRESS       0x08004000
