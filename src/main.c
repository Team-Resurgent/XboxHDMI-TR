// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "stm32f0xx_hal.h"
#include "adv7511_i2c.h"
#include "smbus_i2c.h"
#include "adv7511.h"
#include "debug.h"
#include "led.h"
#include "xbox.h"

UART_HandleTypeDef huart;
adv7511 encoder;
enum xbox_encoder xbox_encoder;

void SystemClock_Config(void);
static void init_gpio(void);

uint8_t init_adv() {
    adv7511_i2c_init();
    smbus_i2c_init();

    HAL_Delay(200);
    debug_log("\r\nADV7511 Chip Revision %u\r\n", adv7511_read_register(0x00));

    uint8_t error = 0;

    //Initialise the encoder object
    adv7511_struct_init(&encoder);

    //Force Hot Plug Detect High
    encoder.hot_plug_detect = 1;
    error |= adv7511_write_register(0xD6, 0b11000000);

    //Power up the encoder and set fixed registers
    error |= adv7511_power_up(&encoder);
    HAL_Delay(200);

    //Set video input mode to YCbCr 444, 12bit databus DDR.
    error |= adv7511_update_register(0x15, 0b00001111, 0b000000101); //ID=5

    //Set video style to style 1 (Y[3:0] Cb[7:0] first edge, Cr[7:0] Y[7:4] second edge)
    error |= adv7511_update_register(0x16, 0b00001100, 0b00001000); //style 1 01 = style 2    10 = style 1  11 = style 3

    //Set DDR Input Edge  first half of pixel data clocking edge, Bit 1 |= 0 for falling edge, 1 for rising edge CHECK
    error |= adv7511_update_register(0x16, 0b00000010, 0b00000010); //Rising

    if (xbox_encoder == ENCODER_XCALIBUR)
    {
        //Bit order reverse for input signals. 1 |= LSB .... MSB Reverse Bus Order
        //Just how my PCB is layed out.
        error |= adv7511_update_register(0x48, 0b01000000, 0b00000000);

        //DDR Alignment . 1 |= DDR input is D[35:18] (left aligned), 0 = right aligned
        error |= adv7511_update_register(0x48, 0b00100000, 0b00100000);

        // No Delay adjust for xcalibur
    } else {
        //Bit order reverse for input signals. 1 |= LSB .... MSB Reverse Bus Order
        //Just how my PCB is layed out.
        error |= adv7511_update_register(0x48, 0b01000000, 0b01000000);

        //DDR Alignment . 1 |= DDR input is D[35:18] (left aligned), 0 = right aligned
        error |= adv7511_update_register(0x48, 0b00100000, 0b00000000);

        //Clock Delay adjust.
        error |= adv7511_update_register(0xD0, 0b10000000, 0b10000000);
        error |= adv7511_update_register(0xD0, 0b01110000, 3 << 4); //0 to 6, 3 = no delay
        error |= adv7511_update_register(0xBA, 0b11100000, 3 << 5);
    }

    //Must be 11 for ID=5 (No sync pulse)
    error |= adv7511_update_register(0xD0, 0b00001100, 0b00001100);

    //Enable DE generation. This is derived from HSYNC,VSYNC for video active framing
    error |= adv7511_update_register(0x17, 0b00000001, 0b00000001);

    //Set Output Format to 4:4:4
    error |= adv7511_update_register(0x16, 0b10000000, 0b00000000);

    //Start AVI Infoframe Update
    error |= adv7511_update_register(0x4A, 0b01000000, 0b01000000);

    //Infoframe output format to YCbCr 4:4:4 in infoframe* 10=YCbCr4:4:4, 00=RGB
    error |= adv7511_update_register(0x55, 0b01100000, 0b01000000);

    //Infoframe output aspect ratio default to 4:3
    error |= adv7511_update_register(0x56, 0b00110000, 0b00010000);
    error |= adv7511_update_register(0x56, 0b00001111, 0b00001000);

    //END AVI Infoframe Update
    error |= adv7511_update_register(0x4A, 0b01000000, 0b00000000);

    //Output Color Space Selection 0 = RGB 1 = YCbCr
    error |= adv7511_update_register(0x16, 0b00000001, 0b00000001);

    //Set Output to HDMI Mode (Instead of DVI Mode)
    error |= adv7511_update_register(0xAF, 0b00000010, 0b00000010);

    //Enable General Control Packet CHECK
    error |= adv7511_update_register(0x40, 0b10000000, 0b10000000);

    //Disable CSC
    //error |= adv7511_update_register(0x18, 0xFF, 0x00);

    //SETUP AUDIO
    //Set 48kHz Audio clock CHECK (N Value)
    error |= adv7511_update_register(0x01, 0xFF, 0x00);
    error |= adv7511_update_register(0x02, 0xFF, 0x18);
    error |= adv7511_update_register(0x03, 0xFF, 0x00);

    //Set SPDIF audio source
    error |= adv7511_update_register(0x0A, 0b01110000, 0b00010000);

    //SPDIF enable
    error |= adv7511_update_register(0x0B, 0b10000000, 0b10000000);
    return error;
}

uint8_t set_video_mode(uint8_t mode, bool wide, bool interlaced){
    if (mode > XBOX_VIDEO_1080i) {
        return 1;
    }

    video_setting vs;
    switch (xbox_encoder) {
        case ENCODER_CONEXANT:
            vs = video_settings_conexant[mode];
            break;

        case ENCODER_FOCUS:
            vs = video_settings_focus[mode];
            break;

        case ENCODER_XCALIBUR:
            vs = video_settings_xcalibur[mode];
            break;

        default:
            return 1;
    }

    uint8_t error = 0;
    uint8_t ddr_edge = 1;

    if (wide)
    {
        //Infoframe output aspect ratio default to 16:9
        error |= adv7511_update_register(0x56, 0b00110000, 0b00100000);
        debug_log("Set wide aspect\r\n");
    }
    else
    {
        //Infoframe output aspect ratio default to 4:3
        error |= adv7511_update_register(0x56, 0b00110000, 0b00010000);
        debug_log("Set 4:3 aspect\r\n");
    }

    if (interlaced) {
        //Set interlace offset
        error |= adv7511_update_register(0x37, 0b11100000, 0 << 5);
        //Offset for Sync Adjustment Vsync Placement
        error |= adv7511_update_register(0xDC, 0b11100000, 0 << 5);
    }

    error |= adv7511_update_register(0x16, 0b00000010, ddr_edge << 1);
    error |= adv7511_update_register(0x36, 0b00111111, (uint8_t)vs.vs_delay);
    error |= adv7511_update_register(0x35, 0b11111111, (uint8_t)(vs.hs_delay >> 2));
    error |= adv7511_update_register(0x36, 0b11000000, (uint8_t)(vs.hs_delay << 6));
    error |= adv7511_update_register(0x37, 0b00011111, (uint8_t)(vs.active_w >> 7));
    error |= adv7511_update_register(0x38, 0b11111110, (uint8_t)(vs.active_w << 1));
    error |= adv7511_update_register(0x39, 0b11111111, (uint8_t)(vs.active_h >> 4));
    error |= adv7511_update_register(0x3A, 0b11110000, (uint8_t)(vs.active_h << 4));
    debug_log("Actual Pixel Repetition : 0x%02x\r\n", (adv7511_read_register(0x3D) & 0xC0) >> 6);
    debug_log("Actual VIC Sent : 0x%02x\r\n", adv7511_read_register(0x3D) & 0x1F);

    return error;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    debug_init();
    init_gpio();
    led_init();

#ifdef XCALIBUR
    xbox_encoder = ENCODER_XCALIBUR;
#else
    xbox_encoder = ENCODER_CONEXANT;
#endif

    uint8_t error = init_adv();

    if (error)
    {
        debug_log("Encountered error when setting up ADV7511\r\n");
    }

    apply_csc((uint8_t *)identityMatrix);
    uint8_t vic = 0x80;
    uint8_t pll_lock = 0;

    while (1)
    {
        if (error)
        {
            debug_log("Encountered error when setting up ADV7511\r\n");
            error = 0;
        }

        //Check PLL status
        pll_lock = (adv7511_read_register(0x9E) >> 4) & 0x01;
        if (pll_lock == 0)
        {
            debug_log("PLL Lock: %u\r\n", pll_lock);
            led_status1(false);
        }
        else
        {
            led_status1(true);
        }

        if ((adv7511_read_register(0x3e) >> 2) != (vic & 0x0F))
        {
            debug_log("VIC Changed!\r\n");
            //Set MSB to 1. This indicates a recent change.
            vic = ADV7511_VIC_CHANGED | adv7511_read_register(0x3e) >> 2;
            debug_log("Detected VIC#: 0x%02x\r\n", vic & ADV7511_VIC_CHANGED_CLEAR);
        }

        if (encoder.interrupt)
        {
            uint8_t interrupt_register = adv7511_read_register(0x96);
            debug_log("Interrupt occurred!\r\n");
            debug_log("interrupt_register: 0x%02x\r\n", interrupt_register);

            if (interrupt_register & ADV7511_INT0_HPD)
            {
                debug_log("HPD interrupt\r\n");
                encoder.hot_plug_detect = (adv7511_read_register(0x42) >> 6) & 0x01;
            }

            if (interrupt_register & ADV7511_INT0_MONITOR_SENSE)
            {
                debug_log("Monitor Sense Interrupt\r\n");
                encoder.monitor_sense = (adv7511_read_register(0x42) >> 5) & 0x01;
            }

            (encoder.hot_plug_detect) ? printf("HDMI cable detected\r\n") : printf("HDMI cable not detected\r\n");
            (encoder.monitor_sense) ? printf("Monitor is ready\r\n") : printf("Monitor is not ready\r\n");

            if (encoder.hot_plug_detect && encoder.monitor_sense)
            {
                adv7511_power_up(&encoder);
            }

            encoder.interrupt = 0;
            //Re-enable interrupts
            adv7511_update_register(0x96, 0b11000000, 0xC0);
        }

        if (vic & ADV7511_VIC_CHANGED)
        {
            vic &= ADV7511_VIC_CHANGED_CLEAR;

            //Set pixel rep mode to auto
            //error |= adv7511_update_register(0x3B, 0b01100000, 0b00000000);

            if (vic == ADV7511_VIC_VGA_640x480_4_3)
            {
                debug_log("Set timing for VGA 640x480 4:3\r\n");
                error |= set_video_mode(XBOX_VIDEO_VGA, false, false);
            }

            else if (vic == ADV7511_VIC_480p_4_3 || vic == ADV7511_VIC_480p_16_9 || vic == ADV7511_VIC_UNAVAILABLE)
            {
                debug_log("Set timing for 480p\r\n");
                error |= set_video_mode(XBOX_VIDEO_480p, (vic == ADV7511_VIC_480p_16_9), false);
            }

            else if (vic == ADV7511_VIC_720p_60_16_9)
            {
                debug_log("Set timing for 720p 16:9\r\n");
                error |= set_video_mode(XBOX_VIDEO_720p, true, false);
            }

            else if (vic == ADV7511_VIC_1080i_60_16_9)
            {
                debug_log("Set timing for 1080i 16:9\r\n");
                error |= set_video_mode(XBOX_VIDEO_1080i, false, true);
            }
        }
    }
}

//TODO split this out
static void init_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    //EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
}

void _Error_Handler(char *file, int line)
{
    debug_log("Error in file %s at line %d\n", file, line);
    while (1) {}
}
