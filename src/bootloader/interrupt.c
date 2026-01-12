#include "stm32f0xx_hal.h"
#include "../shared/debug.h"
#include "../shared/defines.h"

// Get application's interrupt handlers from application's vector table
// Vector table structure:
// [0] = Initial Stack Pointer
// [1] = Reset Handler
// [2] = NMI Handler
// [3] = HardFault Handler
// ...
// [15] = SysTick Handler (exception #15)
// [21] = EXTI0_1_IRQHandler (IRQ #5)
// [40] = I2C2_IRQHandler (IRQ #24)

extern uint8_t bootloader_running;
extern I2C_HandleTypeDef hi2c2;

void SysTick_Handler(void)
{
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_systick_addr = app_vector_table[15];

    if (bootloader_running == 0 && app_systick_addr != 0xFFFFFFFF && app_systick_addr != 0x00000000) {
        void (*app_systick_handler)(void) = (void (*)(void))app_systick_addr;
        app_systick_handler();
    } else {
        HAL_IncTick();
        HAL_SYSTICK_IRQHandler();
    }
}

void HardFault_Handler(void)
{
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_hardfault_addr = app_vector_table[3];

    if (bootloader_running == 0 && app_hardfault_addr != 0xFFFFFFFF && app_hardfault_addr != 0x00000000) {
        void (*app_hardfault_handler)(void) = (void (*)(void))app_hardfault_addr;
        app_hardfault_handler();
    } else {
        debug_log("HardFault occured!\n");
        while (1);
    }
}

void EXTI0_1_IRQHandler(void)
{
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_exti_addr = app_vector_table[21];

    if (bootloader_running == 0 && app_exti_addr != 0xFFFFFFFF && app_exti_addr != 0x00000000) {
        void (*app_exti_handler)(void) = (void (*)(void))app_exti_addr;
        app_exti_handler();
    }
}

// Flag to indicate if bootloader is using I2C2
// Set this to 1 in bootloader code when initializing I2C2
// Set to 0 before jumping to application
// Default to 0 (bootloader not using I2C)
// volatile uint8_t bootloader_i2c2_active = 0;

// // Forward declaration for bootloader's I2C handle
// // Define this in your bootloader code if using I2C2
// extern I2C_HandleTypeDef *bootloader_hi2c2;

void I2C2_IRQHandler(void) {
    // Otherwise, forward to application
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_i2c2_addr = app_vector_table[40];

    if (bootloader_running == 0 && bootloader_running == 0 && app_i2c2_addr != 0xFFFFFFFF && app_i2c2_addr != 0x00000000) {
        void (*app_i2c2_handler)(void) = (void (*)(void))app_i2c2_addr;
        app_i2c2_handler();
    } else {
        if (hi2c2.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR | I2C_FLAG_TIMEOUT | I2C_FLAG_ALERT | I2C_FLAG_PECERR)) {
            HAL_I2C_ER_IRQHandler(&hi2c2);
        } else {
            HAL_I2C_EV_IRQHandler(&hi2c2);
        }
    }
}
