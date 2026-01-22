#include "adv7511_xbox.h"

void init_adv(adv7511 *encoder, const xbox_encoder xb_encoder) {
    adv7511_i2c_init();

    // Initialise the encoder object
    adv7511_struct_init(encoder);

    // [7:6] HPD Control (forced to high)
    // [5] Fixed 0
    // [4] TMDS Clock soft turn on
    // [3:1] Fixed 000
    // [0] AV gating off
    adv7511_write_register(0xD6, 0b11010000);

    // Power up the encoder and set fixed registers
    adv7511_power_up(encoder);
    HAL_Delay(50);

    // [3:0] Set video input mode to RGB/YCbCr 4:4:4, 12bit databus DDR
    // [7:4] Audio to 48kHz
    adv7511_write_register(0x15, 0b00100101);

    // [7] Output Format 4:4:4
    // [5:4] 8 bit video
    // [3:2] video style 1 (Y[3:0] Cb[7:0] first edge, Cr[7:0] Y[7:4] second edge)
    // [1] Set DDR Input Rising Edge
    // [0] YCbCr
    adv7511_write_register(0x16, 0b00111011);

    update_avi_infoframe(false);

    // Setup xbox encoder specific stuff (Xcalibur uses different settings)
    init_adv_encoder_specific(xb_encoder);

    // [0] Enable DE generation. This is derived from HSYNC,VSYNC for video active framing
    adv7511_update_register(0x17, 0b00000001, 0b00000001);

    // Set Output to HDMI Mode (Instead of DVI Mode)
    // [7] HDCP Disabled
    // [6:5] Must be set to default value 00
    // [4] Frame encryption
    // [3:2] Must be set to default value 01
    // [1] HDMI/DVI
    // [0] Must be set to default 0
    adv7511_write_register(0xAF, 0b00000110);

    // [7] Enable General Control Packet CHECK
    adv7511_update_register(0x40, 0b10000000, 0b10000000);

    init_adv_audio();
}

void init_adv_encoder_specific(const xbox_encoder xb_encoder) {
    if (xb_encoder == ENCODER_XCALIBUR) {
        // [6] Normal Bus Order, [5] DDR Alignment D[35:18] (left aligned)
        adv7511_update_register(0x48, 0b01100000, 0b00100000);
        // [7] Disable DDR Negative Edge CLK Delay, [6:4] with -400ps delay
        // [3:2] No sync pulse, [1] Data enable, then sync, [0] Fixed
        adv7511_write_register(0xD0, 0b00101110);
        // [7:5] -0.8ns clock delay
        adv7511_update_register(0xBA, 0b11100000, 0b00100000);
    } else {
        // [6] LSB .... MSB Reverse Bus Order, [5] DDR Alignment D[17:0] (right aligned)
        adv7511_update_register(0x48, 0b01100000, 0b01000000);
        // [7] Enable DDR Negative Edge CLK Delay, [6:4] with 0ps delay
        // [3:2] No sync pulse, [1] Data enable, then sync, [0] Fixed
        adv7511_write_register(0xD0, 0b10111110);
        // [7:5] No clock delay
        adv7511_update_register(0xBA, 0b11100000, 0b01100000);
    }
}

void init_adv_audio() {
    // [19:0] Set 48kHz Audio clock CHECK (N Value)
    adv7511_write_register(0x01, 0x00);
    adv7511_write_register(0x02, 0x18);
    adv7511_write_register(0x03, 0x00);

    // [6:4] Set SPDIF audio source
    adv7511_update_register(0x0A, 0b01110000, 0b00010000);

    // [7] SPDIF enable
    adv7511_update_register(0x0B, 0b10000000, 0b10000000);
}

void update_avi_infoframe(const bool widescreen) {
    // [6] Start AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b01000000);
    // [6:5] Infoframe output format to YCbCr4:4:4
    adv7511_update_register(0x55, 0b01100000, 0b01000000);
    // [5:4] Set aspect ratio
    // [3:0] Active format aspect ratio, same as aspect ratio
    adv7511_write_register(0x56, widescreen ? 0b00101000 : 0b00011000);
    // [6] End AVI Infoframe Update
    adv7511_update_register(0x4A, 0b01000000, 0b00000000);
}
