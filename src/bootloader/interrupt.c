#include "stm32f0xx_hal.h"
#include "../shared/debug.h"
#include "../shared/defines.h"

// Get application's SysTick handler from application's vector table
// Vector table structure:
// [0] = Initial Stack Pointer
// [1] = Reset Handler
// [2] = NMI Handler
// [3] = HardFault Handler
// ...
// [15] = SysTick Handler (exception #15)

void SysTick_Handler(void)
{
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_systick_addr = app_vector_table[15];
    
    if (app_systick_addr != 0xFFFFFFFF && app_systick_addr != 0x00000000) {
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
    
    if (app_hardfault_addr != 0xFFFFFFFF && app_hardfault_addr != 0x00000000) {
        void (*app_hardfault_handler)(void) = (void (*)(void))app_hardfault_addr;
        app_hardfault_handler();
    } else {
        debug_log("HardFault occured!\n");
        while (1);
    }
}