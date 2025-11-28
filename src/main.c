// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "stm32f0xx_hal.h"
#include "adv7511_i2c.h"
#include "smbus_i2c.h"
#include "adv7511.h"
#include "debug.h"
#include "led.h"
#include "xbox_video.h"

adv7511 encoder;
xbox_encoder xb_encoder;

void init_adv();
void init_adv_encoder_specific();
void adv_handle_interrupts();

void bios_loop();
void stand_alone_loop();

void set_video_mode_vic(const uint8_t mode, const bool wide, const bool interlaced);
void set_video_mode_bios(const uint32_t mode, const video_region region);
void set_adv_video_mode(const video_setting * const vs, const bool widescreen, const bool interlaced);

void SystemClock_Config(void);
static void init_gpio(void);

int main(void)
{
    // TODO Allow user to force any of the 3 encoders
#ifdef XCALIBUR
    xb_encoder = ENCODER_XCALIBUR;
#elif FOCUS
    xb_encoder = ENCODER_FOCUS;
#else
    xb_encoder = ENCODER_CONEXANT;
#endif

    HAL_Init();
    SystemClock_Config();

    debug_init();
    init_gpio();
    led_init();

    init_adv();
    smbus_i2c_init();

    apply_csc((uint8_t *)identityMatrix);

    while (true)
    {
        debug_ring_flush();

        // Check PLL status
        bool pll_lock = (adv7511_read_register(0x9E) >> 4) & 0x01;
        led_status1(pll_lock);

        if (bios_tookover()) {
            bios_loop();
        }
        else
        {
            stand_alone_loop();
        }

        adv_handle_interrupts();
    }
}

void init_adv()
{
    adv7511_i2c_init();

    HAL_Delay(50);

    // Initialise the encoder object
    adv7511_struct_init(&encoder);

    // Setup HPD Control (forced to high)
    adv7511_write_register(0xD6, 0b11000000);

    // Power up the encoder and set fixed registers
    adv7511_power_up(&encoder);
    HAL_Delay(50);

    // Set video input mode to YCbCr 444, 12bit databus DDR
    adv7511_write_register(0x15, 0b000000101);

    // Set Output Format to 4:4:4
    // 8 bit video
    // Set video style to style 1 (Y[3:0] Cb[7:0] first edge, Cr[7:0] Y[7:4] second edge)
    // Set DDR Input Rising Edge
    adv7511_write_register(0x16, 0b00111011);

    // Setup encoder specific stuff
    init_adv_encoder_specific();

    // Enable DE generation. This is derived from HSYNC,VSYNC for video active framing
    adv7511_update_register(0x17, 0b00000001, 0b00000001);

    // Start AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b01000000);

    // Infoframe output format to YCbCr 4:4:4 in infoframe* 10=YCbCr4:4:4, 00=RGB
    adv7511_update_register(0x55, 0b01100000, 0b01000000);

    // Infoframe output aspect ratio default to 4:3 + Active Format Aspect Ratio (same as aspect ratio)
    adv7511_write_register(0x56, 0b00011000);

    // END AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b00000000);

    // Set Output to HDMI Mode (Instead of DVI Mode)
    adv7511_write_register(0xAF, 0b00000010);

    // Enable General Control Packet CHECK
    adv7511_update_register(0x40, 0b10000000, 0b10000000);

    // SETUP AUDIO
    // Set 48kHz Audio clock CHECK (N Value)
    adv7511_write_register(0x01, 0x00);
    adv7511_write_register(0x02, 0x18);
    adv7511_write_register(0x03, 0x00);

    // Set SPDIF audio source
    adv7511_update_register(0x0A, 0b01110000, 0b00010000);

    // SPDIF enable
    adv7511_update_register(0x0B, 0b10000000, 0b10000000);
}

void init_adv_encoder_specific()
{
    if (xb_encoder == ENCODER_XCALIBUR)
    {
        // Normal Bus Order, DDR Alignment D[35:18] (left aligned)
        adv7511_write_register(0x48, 0b00100000);
        // Disable DDR Negative Edge CLK Delay, with 0ns delay. No sync pulse
        adv7511_write_register(0xD0, 0b00111100);
        // -0.4ns clock delay
        adv7511_write_register(0xBA, 0b01000000);
        return;
    }

    if (xb_encoder == ENCODER_FOCUS) {
        // LSB .... MSB Reverse Bus Order, DDR Alignment D[17:0] (right aligned)
        adv7511_write_register(0x48, 0b01000000);
        // Enable DDR Negative Edge CLK Delay, with 0ns delay. No sync pulse
        adv7511_write_register(0xD0, 0b10111100);
        // -0.4ns clock delay
        adv7511_write_register(0xBA, 0b01000000);
        return;
    }

    if (xb_encoder == ENCODER_CONEXANT) {
        // LSB .... MSB Reverse Bus Order, DDR Alignment D[17:0] (right aligned)
        adv7511_write_register(0x48, 0b01000000);
        // Enable DDR Negative Edge CLK Delay, with 0ns delay. No sync pulse
        adv7511_write_register(0xD0, 0b10111100);
        // No clock delay
        adv7511_write_register(0xBA, 0b01100000);
    }
}

inline void bios_loop()
{
    led_status2(true);

    if (video_mode_updated()) {
        const SMBusSettings * const vid_settings = getSMBusSettings();
        // Detect the encoder, if it changed reinit encoder specific values
        if (xb_encoder != vid_settings->encoder) {
            xb_encoder = vid_settings->encoder;
            init_adv_encoder_specific();
        }

        set_video_mode_bios(vid_settings->mode, vid_settings->region);
        ack_video_mode_update();
    }
}

inline void stand_alone_loop()
{
    led_status2(false);

    if ((adv7511_read_register(0x3e) >> 2) != (encoder.vic & 0x0F))
    {
        // Set MSB to 1. This indicates a recent change.
        encoder.vic = ADV7511_VIC_CHANGED | adv7511_read_register(0x3e) >> 2;
        debug_log("Detected VIC#: 0x%02x\r\n", encoder.vic & ADV7511_VIC_CHANGED_CLEAR);
    }

    if (encoder.vic & ADV7511_VIC_CHANGED)
    {
        encoder.vic &= ADV7511_VIC_CHANGED_CLEAR;

        if (encoder.vic == VIC_01_VGA_640x480_4_3)
        {
            set_video_mode_vic(XBOX_VIDEO_VGA, false, false);
        }
        else if (encoder.vic == VIC_02_480p_60__4_3 || encoder.vic == VIC_00_VIC_Unavailable)
        {
            set_video_mode_vic(XBOX_VIDEO_480p_640, false, false);
        }
        else if (encoder.vic == VIC_03_480p_60_16_9)
        {
            set_video_mode_vic(XBOX_VIDEO_480p_720, true, false);
        }
        else if (encoder.vic == VIC_04_720p_60_16_9)
        {
            set_video_mode_vic(XBOX_VIDEO_720p, true, false);
        }
        else if (encoder.vic == VIC_05_1080i_60_16_9)
        {
            set_video_mode_vic(XBOX_VIDEO_1080i, true, true);
        }
    }
}

void adv_handle_interrupts()
{
    if (encoder.interrupt)
    {
        uint8_t interrupt_register = adv7511_read_register(0x96);

        if (interrupt_register & ADV7511_INT0_HPD)
        {
            // debug_log("HPD interrupt\r\n");
            encoder.hot_plug_detect = (adv7511_read_register(0x42) >> 6) & 0x01;
        }

        if (interrupt_register & ADV7511_INT0_MONITOR_SENSE)
        {
            // debug_log("Monitor Sense Interrupt\r\n");
            encoder.monitor_sense = (adv7511_read_register(0x42) >> 5) & 0x01;
        }

        if (encoder.hot_plug_detect && encoder.monitor_sense)
        {
            adv7511_power_up(&encoder);
        }

        encoder.interrupt = 0;
        // Re-enable interrupts
        adv7511_update_register(0x96, 0b11000000, 0xC0);
    }
}

void set_video_mode_bios(const uint32_t mode, const video_region region)
{
    const video_setting* vs = NULL;
    switch (xb_encoder) {
        case ENCODER_CONEXANT:
            for (int i = 0; i < XBOX_VIDEO_BIOS_MODE_COUNT; ++i) {
                if (video_settings_conexant_bios[i].mode == mode) {
                    vs = &video_settings_conexant_bios[i].vs;
                    break;
                }
            }
            break;

        case ENCODER_FOCUS:
            for (int i = 0; i < XBOX_VIDEO_BIOS_MODE_COUNT; ++i) {
                if (video_settings_focus_bios[i].mode == mode) {
                    vs = &video_settings_focus_bios[i].vs;
                    break;
                }
            }
            break;

        case ENCODER_XCALIBUR:
            for (int i = 0; i < XBOX_VIDEO_BIOS_MODE_COUNT; ++i) {
                if (video_settings_xcalibur_bios[i].mode == mode) {
                    vs = &video_settings_xcalibur_bios[i].vs;
                    break;
                }
            }
            break;

        default:
            break;
    }

    // TODO 50Hz is not applied

    // We didnt find a video mode, abort change
    if (vs == NULL) {
        debug_log("Video mode not present %d\r\n", mode);
        return;
    }

    bool widescreen = mode & XBOX_VIDEO_WIDESCREEN;
    bool interlaced = (mode == 0x880E0C03); // The only mode that is interlaced on the bus is 1080i

    // Force pixel repeat to 1
    adv7511_write_register(0x3B, 0b01100000);
    set_adv_video_mode(vs, widescreen, interlaced);
}

void set_video_mode_vic(const uint8_t mode, const bool widescreen, const bool interlaced)
{
    if (mode > XBOX_VIDEO_1080i) {
        debug_log("Invalid video mode for VIC\r\n");
        return;
    }

    const video_setting* vs = NULL;
    switch (xb_encoder) {
        case ENCODER_CONEXANT:
            vs = &video_settings_conexant[mode];
            break;

        case ENCODER_FOCUS:
            vs = &video_settings_focus[mode];
            break;

        case ENCODER_XCALIBUR:
            vs = &video_settings_xcalibur[mode];
            break;

        default:
            debug_log("Invalid encoder in set_video_mode_vic\r\n");
            return;
    }

    debug_log("Set %d mode, widescree %s, interlaced %s\r\n", mode, widescreen ? "true" : "false", interlaced ? "true" : "false");
    set_adv_video_mode(vs, widescreen, interlaced);
}

inline void set_adv_video_mode(const video_setting * const vs, const bool widescreen, const bool interlaced)
{
    if (widescreen) {
        adv7511_write_register(0x56, 0b00101000); // 16:9
    } else {
        adv7511_write_register(0x56, 0b00011000); // 4:3
    }

    if (interlaced) {
        // Set interlace offset
        adv7511_update_register(0x37, 0b11100000, 0 << 5);
        // Offset for Sync Adjustment Vsync Placement
        adv7511_update_register(0xDC, 0b11100000, 0 << 5);
    }

    adv7511_write_register(0x35, (uint8_t)(vs->delay_hs >> 2));
    adv7511_write_register(0x36, ((0b00111111 & (uint8_t)vs->delay_vs)) | (0b11000000 & (uint8_t)(vs->delay_hs << 6)));
    adv7511_update_register(0x37, 0b00011111, (uint8_t)(vs->active_w >> 7)); // 0x37 is shared with interlaced
    adv7511_write_register(0x38, (uint8_t)(vs->active_w << 1));
    adv7511_write_register(0x39, (uint8_t)(vs->active_h >> 4));
    adv7511_write_register(0x3A, (uint8_t)(vs->active_h << 4));
    // Hsync/Vsync duration+porch might be needed at some point, 1.6 FPAR has some pillar boxing

    // Set the vic from the table
    adv7511_write_register(0x3C, vs->vic);
    debug_log("Actual Pixel Repetition : 0x%02x\r\n", (adv7511_read_register(0x3D) & 0xC0) >> 6);
    debug_log("Actual VIC Sent : 0x%02x\r\n", adv7511_read_register(0x3D) & 0x1F);
}

// TODO split this out
static void init_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
}

void _Error_Handler(char *file, int line)
{
    debug_log("Error in file %s at line %d\n", file, line);
    while (1) {}
}
