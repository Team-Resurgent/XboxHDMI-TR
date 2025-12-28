// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "stm32f0xx_hal.h"
#include "adv7511_i2c.h"
#include "smbus_i2c.h"
#include "adv7511.h"
#include "debug.h"
#include "led.h"
#include "xbox_video_bios.h"
#include "xbox_video_standalone.h"

adv7511 encoder;

// Allow user to force any of the 3 encoders, only required for vic mode
#ifdef BUILD_XCALIBUR
    xbox_encoder xb_encoder = ENCODER_XCALIBUR;
#elif BUILD_FOCUS
    xbox_encoder xb_encoder = ENCODER_FOCUS;
#else
    xbox_encoder xb_encoder = ENCODER_CONEXANT;
#endif

void init_adv();
void init_adv_encoder_specific();
void init_adv_audio();
void adv_handle_interrupts();
void adv_disable_video();
void adv_enable_video();

void bios_loop();
void stand_alone_loop();

void update_avi_infoframe(const bool widescreen);
void set_video_mode_vic(const uint8_t mode, const bool wide, const bool interlaced);
void set_video_mode_bios(const uint32_t mode, const uint32_t avinfo, const video_region region);
void set_adv_video_mode_bios(const VideoMode video_mode, const bool widescreen, const bool rgb);
uint8_t get_vic_from_video_mode(const VideoMode * const vm, const bool widescreen);

void SystemClock_Config(void);
static void init_gpio(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    debug_init();
    init_gpio();
    led_init();

    init_adv();
    smbus_i2c_init();

    while (true)
    {
        debug_ring_flush();

        // Check PLL status
        bool pll_lock = (adv7511_read_register(0x9E) >> 4) & 0x01;
        set_led_1(pll_lock);

        adv_handle_interrupts();

        if (bios_took_over()) {
            set_led_2(true);
            bios_loop();
        }
        else
        {
            set_led_2(false);
            stand_alone_loop();
        }
    }
}

inline void init_adv()
{
    adv7511_i2c_init();

    // Initialise the encoder object
    adv7511_struct_init(&encoder);

    // Setup HPD Control (forced to high)
    adv7511_write_register(0xD6, 0b11000000);

    // Power up the encoder and set fixed registers
    adv7511_power_up(&encoder);
    HAL_Delay(50);

    // Set video input mode to RGB/YCbCr 4:4:4, 12bit databus DDR. Audio to 48kHz
    adv7511_write_register(0x15, 0b00100101);

    // Set Output Format to 4:4:4
    // 8 bit video
    // Set video style to style 1 (Y[3:0] Cb[7:0] first edge, Cr[7:0] Y[7:4] second edge)
    // Set DDR Input Rising Edge
    adv7511_write_register(0x16, 0b00111011);

    update_avi_infoframe(false);

    // Setup encoder specific stuff
    init_adv_encoder_specific();

    // Enable DE generation. This is derived from HSYNC,VSYNC for video active framing
    adv7511_update_register(0x17, 0b00000001, 0b00000001);

    // Set Output to HDMI Mode (Instead of DVI Mode)
    adv7511_write_register(0xAF, 0b00000010);

    // Enable General Control Packet CHECK
    adv7511_update_register(0x40, 0b10000000, 0b10000000);

    init_adv_audio();

    // Set up the color space correction for RGB signals, disabled by default
    apply_csc((uint8_t *)CscRgbToYuv601);
}

void init_adv_encoder_specific()
{
    if (xb_encoder == ENCODER_XCALIBUR)
    {
        // Normal Bus Order, DDR Alignment D[35:18] (left aligned)
        adv7511_write_register(0x48, 0b00100000);
        // Disable DDR Negative Edge CLK Delay, with 0ps delay
        // No sync pulse. Data enable, then sync
        adv7511_write_register(0xD0, 0b00111110);
        // -0.4ns clock delay
        adv7511_write_register(0xBA, 0b01000000);
    } else {
        // LSB .... MSB Reverse Bus Order, DDR Alignment D[17:0] (right aligned)
        adv7511_write_register(0x48, 0b01000000);
        // Enable DDR Negative Edge CLK Delay, with 0ps delay
        // No sync pulse. Data enable, then sync
        adv7511_write_register(0xD0, 0b10111110);
        // No clock delay
        adv7511_write_register(0xBA, 0b01100000);
    }
}

inline void init_adv_audio() {
    // Set 48kHz Audio clock CHECK (N Value)
    adv7511_write_register(0x01, 0x00);
    adv7511_write_register(0x02, 0x18);
    adv7511_write_register(0x03, 0x00);

    // Set SPDIF audio source
    adv7511_update_register(0x0A, 0b01110000, 0b00010000);

    // SPDIF enable
    adv7511_update_register(0x0B, 0b10000000, 0b10000000);
}

inline void adv_disable_video() {
    // Gate ouput
    adv7511_update_register(0xD6, 0b00000001, 0b00000001);
}

inline void adv_enable_video() {
    // Enable ouput
    adv7511_update_register(0xD6, 0b00000001, 0b00000000);
}


inline void bios_loop()
{
    if (video_mode_updated()) {
        const SMBusSettings * const vid_settings = getSMBusSettings();
        // Detect the encoder, if it changed reinit encoder specific values
        if (xb_encoder != vid_settings->encoder) {
            xb_encoder = vid_settings->encoder;
            init_adv_encoder_specific();
        }

        set_video_mode_bios(vid_settings->mode, vid_settings->avinfo, vid_settings->region);
        ack_video_mode_update();
    }
}

inline void stand_alone_loop()
{
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

void update_avi_infoframe(const bool widescreen) {
    // Start AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b01000000);
    // Infoframe output format to YCbCr4:4:4
    adv7511_update_register(0x55, 0b01100000, 0b01000000);
    // Set aspect ratio
    adv7511_write_register(0x56, widescreen ? 0b00101000 : 0b00011000);
    // END AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b00000000);
}

void set_video_mode_vic(const uint8_t mode, const bool widescreen, const bool interlaced)
{
    if (mode > XBOX_VIDEO_1080i) {
        debug_log("Invalid video mode for VIC\r\n");
        return;
    }

    const video_setting_vic* vs = NULL;
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

    debug_log("Set %d mode, widescreen %s, interlaced %s\r\n", mode, widescreen ? "true" : "false", interlaced ? "true" : "false");

    // Make sure CSC is off
    adv7511_update_register(0x18, 0b10000000, 0b00000000);

    adv7511_write_register(0x35, (uint8_t)(vs->delay_hs >> 2));
    adv7511_write_register(0x36, ((0b00111111 & (uint8_t)vs->delay_vs)) | (0b11000000 & (uint8_t)(vs->delay_hs << 6)));
    adv7511_update_register(0x37, 0b00011111, (uint8_t)(vs->active_w >> 7)); // 0x37 is shared with interlaced
    adv7511_write_register(0x38, (uint8_t)(vs->active_w << 1));
    adv7511_write_register(0x39, (uint8_t)(vs->active_h >> 4));
    adv7511_write_register(0x3A, (uint8_t)(vs->active_h << 4));

    update_avi_infoframe(widescreen);

    // For VIC mode
    if (interlaced) {
        // Interlace Offset For DE Generation
        adv7511_update_register(0x37, 0b11100000, 0b00000000);
        // Offset for Sync Adjustment Vsync Placement
        adv7511_write_register(0xDC, 0b00000000);
        // Enable settings
        adv7511_update_register(0x41, 0b00000010, 0b00000010);
    } else {
        // Disable manual sync
        adv7511_update_register(0x41, 0b00000010, 0b00000000);
    }

    // Fixes jumping for 1080i, somehow doing this in the init sequence doesnt stick or gets reset
    adv7511_update_register(0xD0, 0b00000010, 0b00000010);

    // Set the vic from the table
    adv7511_write_register(0x3C, vs->vic);

    debug_log("Actual Pixel Repetition : 0x%02x\r\n", (adv7511_read_register(0x3D) & 0xC0) >> 6);
    debug_log("Actual VIC Sent : 0x%02x\r\n", adv7511_read_register(0x3D) & 0x1F);
}

void set_video_mode_bios(const uint32_t mode, const uint32_t avinfo, const video_region region)
{
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
    const bool interlaced = (avinfo & XBOX_AVINFO_INTERLACED) && (mode_index == 0x0e);

    if (interlaced) {
        video_mode.v_active = video_mode.v_active / 2;
        video_mode.vs_delay = video_mode.vs_delay / 2;
    }

    const bool widescreen = mode & XBOX_VIDEO_MODE_BIT_WIDESCREEN;
    const bool rgb = mode & XBOX_VIDEO_MODE_BIT_SCART;

    set_adv_video_mode_bios(video_mode, widescreen, rgb);
}

inline void set_adv_video_mode_bios(const VideoMode vm, const bool widescreen, const bool rgb)
{
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
