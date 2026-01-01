// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#include "stm32f0xx_hal.h"
#include "main.h"
#include "adv7511.h"
#include "interrupt.h"

extern adv7511 encoder;

void ADV_IRQ_HANDLER(void)
{
    encoder.interrupt = 1;
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}
