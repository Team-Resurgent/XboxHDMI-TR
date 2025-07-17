#include "led.h"

static GPIO_InitTypeDef gpio;

void led_init() 
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);
}

void led_status1(bool state)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_status2(bool state)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}