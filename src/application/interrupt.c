#include "stm32f0xx_hal.h"
#include "../shared/adv7511_minimal.h"
#include "../shared/debug.h"

void SysTick_Handler(void) {
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

void HardFault_Handler(void) {
    debug_log("HardFault occured!\n");
    while (1);
}

extern adv7511 encoder;

void ADV_IRQ_HANDLER(void) {
    encoder.interrupt = 1;
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}
