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
#include "adv7511.h"
#include "smbus_i2c.h"
#include "xbox_video_bios.h"

adv7511 encoder;

// Allow user to force any of the 3 encoders, only required for vic mode
#ifdef BUILD_XCALIBUR
    xbox_encoder xb_encoder = ENCODER_XCALIBUR;
#elif BUILD_FOCUS
    xbox_encoder xb_encoder = ENCODER_FOCUS;
#else
    xbox_encoder xb_encoder = ENCODER_CONEXANT;
#endif

void bios_loop();

void set_video_mode_bios(const uint32_t mode, const uint32_t avinfo, const video_region region);
void set_adv_video_mode_bios(const VideoMode video_mode, const bool widescreen, const bool rgb);
uint8_t get_vic_from_video_mode(const VideoMode * const vm, const bool widescreen);

void SystemClock_Config(void);

int main(void)
{
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

    // Set up the color space correction for RGB signals, disabled by default
    adv7511_apply_csc((uint8_t *)CscRgbToYuv601);

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
            bios_loop();
        }
        else
        {
            set_led_2(false);
            stand_alone_loop(&encoder, xb_encoder);
        }
    }
}

inline void bios_loop() {
    static uint32_t current_mode = 0;
    static uint32_t current_avinfo = 0;

    if (video_mode_updated()) {
        const SMBusSettings * const vid_settings = getSMBusSettings();
        // Detect the encoder, if it changed reinit encoder specific values
        if (xb_encoder != vid_settings->encoder) {
            xb_encoder = vid_settings->encoder;
            init_adv_encoder_specific(xb_encoder);
        }

        const uint32_t mode = vid_settings->mode;
        const uint32_t avinfo = vid_settings->avinfo;

        // Only change the video mode if we actually got a new video mode
        if ((current_mode != mode) || (current_avinfo != avinfo)) {
            adv7511_power_down_tmds();
            set_video_mode_bios(mode, avinfo, vid_settings->region);
            adv7511_power_up_tmds();

            current_avinfo = avinfo;
            current_mode = mode;
        }

        ack_video_mode_update();
    }
}

void set_video_mode_bios(const uint32_t mode, const uint32_t avinfo, const video_region region) {
    const VideoMode* table;
    size_t count;

    switch (xb_encoder) {
        case ENCODER_CONEXANT:
            table = CONEXANT_TABLE;
            count = sizeof(CONEXANT_TABLE) / sizeof(CONEXANT_TABLE[0]);
            break;
        case ENCODER_FOCUS:
            table = FOCUS_TABLE;
            count = sizeof(FOCUS_TABLE) / sizeof(FOCUS_TABLE[0]);
            break;
        case ENCODER_XCALIBUR:
            table = XCALIBUR_TABLE;
            count = sizeof(XCALIBUR_TABLE) / sizeof(XCALIBUR_TABLE[0]);
            break;
        default:
            table = NULL;
            count = 0;
            break;
    }

    uint32_t mode_index = ((mode >> 16) & 0xff);
    if (mode_index < 1 || mode_index > count) {
        debug_log("Video mode not present %d\r\n", mode);
        return;
    }

    VideoMode video_mode = table[mode_index - 1];

    // TODO: Figure out if the avinfo is a reliable source for the interlaced flag
    // const bool interlaced = mode_index == 0x0e;
    // most modes are progressive on the bus...
    const bool interlaced = ((avinfo & XBOX_AVINFO_INTERLACED) || (avinfo & XBOX_AVINFO_FILED)) && (mode_index == 0x0e);

    if (interlaced) {
        video_mode.v_active = video_mode.v_active / 2;
        video_mode.vs_delay = video_mode.vs_delay / 2;
    }

    const bool widescreen = mode & XBOX_VIDEO_MODE_BIT_WIDESCREEN;
    const bool rgb = mode & XBOX_VIDEO_MODE_BIT_SCART;

    set_adv_video_mode_bios(video_mode, widescreen, rgb);
}

inline void set_adv_video_mode_bios(const VideoMode vm, const bool widescreen, const bool rgb) {
    // Force pixel repeat to 1 (for forcing VIC)
    adv7511_write_register(0x3B, 0b01100000);

    // Convert RGB to YCbCr if in RGB mode
    adv7511_update_register(0x18, 0b10000000, rgb ? 0b10000000 : 0b00000000);

    adv7511_write_register(0x35, (uint8_t)(vm.hs_delay >> 2));
    adv7511_write_register(0x36, ((0b00111111 & (uint8_t)vm.vs_delay)) | (0b11000000 & (uint8_t)(vm.hs_delay << 6)));
    adv7511_update_register(0x37, 0b00011111, (uint8_t)(vm.h_active >> 7)); // 0x37 is shared with interlaced
    adv7511_write_register(0x38, (uint8_t)(vm.h_active << 1));
    adv7511_write_register(0x39, (uint8_t)(vm.v_active >> 4));
    adv7511_write_register(0x3A, (uint8_t)(vm.v_active << 4));

    adv7511_write_register(0xD7, (uint8_t)(vm.hsync_placement >> 2));
    adv7511_write_register(0xD8, (uint8_t)(vm.hsync_placement << 6) | (vm.hsync_duration  >> 4));
    adv7511_write_register(0xD9, (uint8_t)(vm.hsync_duration  << 4) | (vm.vsync_placement >> 6));
    adv7511_write_register(0xDA, (uint8_t)(vm.vsync_placement << 2) | (vm.vsync_duration  >> 8));
    adv7511_write_register(0xDB, (uint8_t)(vm.vsync_duration));
    adv7511_write_register(0xDC, (uint8_t)(vm.interlaced_offset << 5));

    // Enable settings
    adv7511_update_register(0x41, 0b00000010, 0b00000010);

    // Fixes jumping for 1080i, somehow doing this in the init sequence doesn't stick or gets reset
    adv7511_update_register(0xD0, 0b00000010, 0b00000010);

    uint8_t vic = get_vic_from_video_mode(&vm, widescreen);

    // Set the vic from the table
    adv7511_write_register(0x3C, vic);

    update_avi_infoframe(widescreen);
}

uint8_t get_vic_from_video_mode(const VideoMode * const vm, const bool widescreen) {
    uint8_t vic;
    switch (vm->h_active) {
        case 640:
        if (vm->v_active == 576) {
            vic = widescreen ? VIC_18_576p_50_16_9 : VIC_17_576p_50__4_3;
        } else {
            vic = widescreen ? VIC_02_480p_60__4_3 : VIC_03_480p_60_16_9;
        }
        break;

        case 720:
        if (vm->v_active == 576) {
            vic = widescreen ? VIC_18_576p_50_16_9 : VIC_17_576p_50__4_3;
        } else {
            vic = widescreen ? VIC_02_480p_60__4_3 : VIC_03_480p_60_16_9;
        }
        break;

        case 1280:
        vic = VIC_04_720p_60_16_9;
        break;

        case 1920:
        vic = VIC_05_1080i_60_16_9;
        break;

        default:
        vic = VIC_00_VIC_Unavailable;
        break;
    }
    return vic;
}
