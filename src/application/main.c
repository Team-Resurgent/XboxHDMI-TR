// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "stm32f0xx_hal.h"
#include "../shared/adv7511_i2c.h"
#include "../shared/adv7511_minimal.h"
#include "../shared/adv7511_xbox.h"
#include "../shared/debug.h"
#include "../shared/xbox_video_standalone.h"
#include "../shared/error_handler.h"
#include "../shared/gpio.h"
#include "smbus_i2c.h"
#include "xbox_video_bios.h"

adv7511 encoder;

extern void SystemClock_Config(void);

int main(void)
{
    // Allow user to force any of the 3 encoders, only required for vic mode
#ifdef BUILD_XCALIBUR
    xbox_encoder xb_encoder = ENCODER_XCALIBUR;
#elif BUILD_FOCUS
    xbox_encoder xb_encoder = ENCODER_FOCUS;
#else
    xbox_encoder xb_encoder = ENCODER_CONEXANT;
#endif

    __enable_irq();

    HAL_Init();
    SystemClock_Config();

    debug_init();
    debug_log("Entering Application...\r\n");

    init_gpio();

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

    init_adv(&encoder, xb_encoder);

    bios_init();

    smbus_i2c_init();

    while (true)
    {
        debug_ring_flush();

        // Check PLL status
        bool pll_lock = (adv7511_read_register(0x9E) >> 4) & 0x01;
        set_led_1(pll_lock);

        adv_handle_interrupts(&encoder);

        if (bios_took_over()) {
            set_led_2(true);
            bios_loop(&xb_encoder);
        }
        else
        {
            set_led_2(false);
            stand_alone_loop(&encoder, xb_encoder);
        }
    }
}
