#include <stdbool.h>

#include "xbox_video_standalone.h"
#include "adv7511_minimal.h"
#include "adv7511_xbox.h"
#include "debug.h"

const video_setting_vic video_settings_conexant[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3},
    {118, 36,  640, 480, VIC_02_480p_60__4_3},
    {118, 36,  720, 480, VIC_03_480p_60_16_9},
    {299, 25, 1280, 720, VIC_04_720p_60_16_9},
    {233, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

const video_setting_vic video_settings_focus[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3},
    {118, 36,  640, 480, VIC_02_480p_60__4_3},
    {118, 36,  720, 480, VIC_03_480p_60_16_9},
    {299, 25, 1280, 720, VIC_04_720p_60_16_9},
    {233, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

const video_setting_vic video_settings_xcalibur[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3},
    { 96, 36,  640, 480, VIC_02_480p_60__4_3},
    { 96, 36,  720, 480, VIC_03_480p_60_16_9},
    {259, 25, 1280, 720, VIC_04_720p_60_16_9},
    {185, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

void stand_alone_loop(adv7511 * encoder, const xbox_encoder xb_encoder) {
    if ((adv7511_read_register(0x3e) >> 2) != (encoder->vic & 0x0F)) {
        // Set MSB to 1. This indicates a recent change.
        encoder->vic = ADV7511_VIC_CHANGED | adv7511_read_register(0x3e) >> 2;
        debug_log("Detected VIC#: 0x%02x\r\n", encoder->vic & ADV7511_VIC_CHANGED_CLEAR);
    }

    if (encoder->vic & ADV7511_VIC_CHANGED) {
        encoder->vic &= ADV7511_VIC_CHANGED_CLEAR;

        if (encoder->vic == VIC_01_VGA_640x480_4_3) {
            set_video_mode_vic(xb_encoder, XBOX_VIDEO_VGA, false, false);
        }
        else if (encoder->vic == VIC_02_480p_60__4_3 || encoder->vic == VIC_00_VIC_Unavailable) {
            set_video_mode_vic(xb_encoder, XBOX_VIDEO_480p_640, false, false);
        }
        else if (encoder->vic == VIC_03_480p_60_16_9) {
            set_video_mode_vic(xb_encoder, XBOX_VIDEO_480p_720, true, false);
        }
        else if (encoder->vic == VIC_04_720p_60_16_9) {
            set_video_mode_vic(xb_encoder, XBOX_VIDEO_720p, true, false);
        }
        else if (encoder->vic == VIC_05_1080i_60_16_9) {
            set_video_mode_vic(xb_encoder, XBOX_VIDEO_1080i, true, true);
        }
    }
}

void set_video_mode_vic(const xbox_encoder xb_encoder, const uint8_t mode, const bool widescreen, const bool interlaced) {
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
