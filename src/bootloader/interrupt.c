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

extern I2C_HandleTypeDef hi2c2;

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

void EXTI0_1_IRQHandler(void)
{
}

void I2C2_IRQHandler(void) {
    if (hi2c2.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR | I2C_FLAG_TIMEOUT | I2C_FLAG_ALERT | I2C_FLAG_PECERR)) {
        HAL_I2C_ER_IRQHandler(&hi2c2);
    } else {
        HAL_I2C_EV_IRQHandler(&hi2c2);
    }
}
