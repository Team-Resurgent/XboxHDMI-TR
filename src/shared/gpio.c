#include "gpio.h"
#include "stm32f0xx_hal.h"

void init_gpio() {
    GPIO_InitTypeDef gpio = {};

    // Recovery jumper
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);

    // ?
    __HAL_RCC_GPIOF_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &gpio);
}

bool recovery_jumper_enabled() {
    return !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);
}
