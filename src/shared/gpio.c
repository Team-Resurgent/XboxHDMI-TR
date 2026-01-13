#include "gpio.h"
#include "stm32f0xx_hal.h"

void init_gpio() {
    GPIO_InitTypeDef gpio = {};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // Leds
    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    // Recovery jumper
    gpio.Pin = GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);

    // ?
    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &gpio);
}

void set_led_1(bool state) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void set_led_2(bool state) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool recovery_jumper_enabled() {
    return !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);
}
