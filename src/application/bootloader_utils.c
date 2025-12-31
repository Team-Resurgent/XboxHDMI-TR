#include "bootloader_utils.h"
#include "../shared/defines.h"
#include "stm32f0xx_hal.h"

// Function to request bootloader mode from application
// Call this function and then reset to enter bootloader
void bootloader_request(void) {
    *BOOTLOADER_FLAG_ADDRESS = BOOTLOADER_MAGIC_VALUE;
    NVIC_SystemReset();
}

// Check if bootloader is active (for application code)
uint8_t bootloader_is_active(void) {
    return (*BOOTLOADER_FLAG_ADDRESS == BOOTLOADER_MAGIC_VALUE);
}

