
#include "interrupt.h"
#include "stm32f0xx_hal.h"
#include "..\shared\debug.h"

void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

void HardFault_Handler(void)
{
    debug_log("HardFault occured!\n");
    while (1);
}