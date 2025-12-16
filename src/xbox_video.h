#ifndef __XBOX_H__
#define __XBOX_H__

#include "adv7511.h"

typedef enum xbox_encoder {
    ENCODER_CONEXANT = 0x8A,
    ENCODER_FOCUS    = 0xD4,
    ENCODER_XCALIBUR = 0xE0
} xbox_encoder;

typedef enum {
    VIDEO_REGION_NTSCM = 0x00000100,
    VIDEO_REGION_NTSCJ = 0x00000200,
    VIDEO_REGION_PAL_I = 0x00000300,
    VIDEO_REGION_PAL_M = 0x00000400
} video_region;

#pragma pack(1)
typedef struct video_setting {
    uint16_t delay_hs;
    uint16_t delay_vs;
    uint16_t active_w;
    uint16_t active_h;
    uint8_t  vic;
} video_setting;

typedef struct bios_mode {
    uint32_t      mode;
    video_setting vs;
} bios_mode;

typedef struct video_sync_setting {
    uint16_t hsync_placement;   // 10 bit
    uint16_t hsync_duration;    // 10 bit
    uint16_t vsync_placement;   // 10 bit
    uint16_t vsync_duration;    // 10 bit
    uint8_t  interlaced_offset; // 3 bit
} video_sync_setting;

typedef struct bios_mode_sync {
    uint32_t           mode;
    video_sync_setting vss;
} bios_mode_sync;

#pragma pack()

#define XBOX_VIDEO_MODE_BIT_WIDESCREEN 0x10000000
#define XBOX_VIDEO_MODE_BIT_SCART      0x20000000 // RGB

#define XBOX_VIDEO_MODE_BIT_MASK       0xC0000000
#define XBOX_VIDEO_MODE_BIT_480SDTV    0x00000000
#define XBOX_VIDEO_MODE_BIT_576SDTV    0x40000000
#define XBOX_VIDEO_MODE_BIT_HDTV       0x80000000
#define XBOX_VIDEO_MODE_BIT_VGA        0xC0000000

#define XBOX_VIDEO_DAC_MASK            0x0F000000
#define XBOX_VIDEO_MODE_MASK           0x0000FFFF

#define XBOX_VIDEO_BIOS_MODE_COUNT      39 // Hard coded so all encoders match up
#define XBOX_VIDEO_BIOS_MODE_SYNC_COUNT 3

// Bios mode values
// Note, we should only care about YPrPb values, so the rest is not present in the table
const bios_mode video_settings_conexant_bios[XBOX_VIDEO_BIOS_MODE_COUNT] = {
    // 640x480
    {0x0801010D, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_NTSC_YPrPb             ()
    {0x1801010D, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_NTSC_YPrPb_16x9        ()
    {0x080F0D12, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_NTSC_YPrPb        ()
    {0x180F0D12, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_FPAR_NTSC_YPrPb_16x9   ()
    {0x48030314, {118, 36,  640,  480, VIC_17_576p_50__4_3}}, // 640x480_PAL_I_YPrPb            ()
    {0x58030314, {118, 36,  640,  480, VIC_18_576p_50_16_9}}, // 640x480_PAL_I_YPrPb_16x9       ()
    {0x48100E18, {118, 36,  640,  480, VIC_17_576p_50__4_3}}, // 640x480_FPAR_PAL_I_YPrPb       ()
    {0x58100E18, {118, 36,  640,  480, VIC_18_576p_50_16_9}}, // 640x480_FPAR_PAL_I_YPrPb_16x9  ()
    {0x08010119, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_PAL_60_YPrPb           ()
    {0x18010119, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_PAL_60_YPrPb_16x9      ()
    {0x080F0D1B, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_PAL_60_YPrPb      ()
    {0x180F0D1B, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_FPAR_PAL_60_YPrPb_16x9 ()
    {0x88070701, {119, 36,  720,  480, VIC_02_480p_60__4_3}}, // 640x480_480P                   ()
    {0x88110F01, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_480P              ()

    {0x04010101, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_TO_NTSC_M_YC
    {0x14010101, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_TO_NTSC_M_YC_16x9
    {0x20010101, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_TO_NTSC_M_RGB
    {0x30010101, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_TO_NTSC_M_RGB_16x9

    // 640x576
    {0x4812101D, {118, 25,  640,  576, VIC_17_576p_50__4_3}}, // 640x576_FPAR_PAL_I_YPrPb       ()
    {0x5812101D, {118, 25,  640,  576, VIC_18_576p_50_16_9}}, // 640x576_FPAR_PAL_I_YPrPb_16x9  ()
    {0x48050516, {118, 25,  640,  576, VIC_17_576p_50__4_3}}, // 640x576_PAL_I_YPrPb            ()
    {0x58050516, {118, 25,  640,  576, VIC_18_576p_50_16_9}}, // 640x576_PAL_I_YPrPb_16x9       ()

    // 720x480
    {0x0802020E, {118, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_NTSC_YPrPb             ()
    {0x1802020E, {118, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_NTSC_YPrPb_16x9        ()
    {0x48040415, {118, 36,  720,  480, VIC_17_576p_50__4_3}}, // 720x480_PAL_I_YPrPb            ()
    {0x58040415, {118, 36,  720,  480, VIC_18_576p_50_16_9}}, // 720x480_PAL_I_YPrPb_16x9       ()
    {0x0802021A, {118, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_PAL_60_YPrPb           ()
    {0x1802021A, {118, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_PAL_60_YPrPb_16x9      ()
    {0x88080801, {119, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_480P                   ()

    {0x04020202, { 95, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_TO_NTSC_M_YC
    {0x14020202, { 95, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_TO_NTSC_M_YC_16x9
    {0x20020202, { 95, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_TO_NTSC_M_RGB
    {0x30020202, { 95, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_TO_NTSC_M_RGB_16x9

    // 720x576
    {0x48060617, {118, 25,  720,  576, VIC_17_576p_50__4_3}}, // 720x576_PAL_I_YPrPb            ()
    {0x58060617, {118, 25,  720,  576, VIC_18_576p_50_16_9}}, // 720x576_PAL_I_YPrPb_16x9       ()

    // 720p
    {0x880B0A02, {299, 25, 1280,  720, VIC_04_720p_60_16_9}}, // 1280x720P_720P                 ()

    // 1080i
    {0x880E0C03, {233, 22, 1920,  540, VIC_05_1080i_60_16_9}}, // 1920x1080I_1080I              ()

    // New modes UNK
    {0x00020072, { 95, 37,  640,  480, VIC_02_480p_60__4_3}},

    // End of table / AV Off
    {0x00000000, {  0,  0,    0,    0, VIC_00_VIC_Unavailable}},
};

const bios_mode_sync video_sync_settings_conexant_bios[XBOX_VIDEO_BIOS_MODE_SYNC_COUNT] = {
    {0x0802020E, {62, 16,  6,  9, 0}},
    {0x880E0C03, {44, 88,  2,  5, 0}}, // 1920x1080I_1080I              ()

    // End of table / AV Off
    {0x00000000, { 0,  0,  0,  0, 0}},
};

const bios_mode video_settings_focus_bios[XBOX_VIDEO_BIOS_MODE_COUNT] = {
    // 640x480
    {0x0801010D, {181, 25,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_NTSC_YPrPb             () // OK e.g Metal Slug 3
    {0x1801010D, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_NTSC_YPrPb_16x9        ()
    {0x080F0D12, {181, 25,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_NTSC_YPrPb        () // OK e.g. 007 Everything or Nothing
    {0x180F0D12, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_FPAR_NTSC_YPrPb_16x9   ()
    {0x48030314, {118, 36,  640,  480, VIC_17_576p_50__4_3}}, // 640x480_PAL_I_YPrPb            ()
    {0x58030314, {118, 36,  640,  480, VIC_18_576p_50_16_9}}, // 640x480_PAL_I_YPrPb_16x9       ()
    {0x48100E18, {118, 36,  640,  480, VIC_17_576p_50__4_3}}, // 640x480_FPAR_PAL_I_YPrPb       ()
    {0x58100E18, {118, 36,  640,  480, VIC_18_576p_50_16_9}}, // 640x480_FPAR_PAL_I_YPrPb_16x9  ()
    {0x08010119, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_PAL_60_YPrPb           ()
    {0x18010119, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_PAL_60_YPrPb_16x9      ()
    {0x080F0D1B, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_PAL_60_YPrPb      ()
    {0x180F0D1B, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_FPAR_PAL_60_YPrPb_16x9 ()
    {0x88070701, {119, 36,  720,  480, VIC_02_480p_60__4_3}}, // 640x480_480P                   ()
    {0x88110F01, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_480P              ()

    {0x04010101, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_TO_NTSC_M_YC
    {0x14010101, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_TO_NTSC_M_YC_16x9
    {0x20010101, {118, 36,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_TO_NTSC_M_RGB
    {0x30010101, {118, 36,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_TO_NTSC_M_RGB_16x9

    // 640x576
    {0x4812101D, {118, 25,  640,  576, VIC_17_576p_50__4_3}}, // 640x576_FPAR_PAL_I_YPrPb       ()
    {0x5812101D, {118, 25,  640,  576, VIC_18_576p_50_16_9}}, // 640x576_FPAR_PAL_I_YPrPb_16x9  ()
    {0x48050516, {118, 25,  640,  576, VIC_17_576p_50__4_3}}, // 640x576_PAL_I_YPrPb            ()
    {0x58050516, {118, 25,  640,  576, VIC_18_576p_50_16_9}}, // 640x576_PAL_I_YPrPb_16x9       ()

    // 720x480
    {0x0802020E, {118, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_NTSC_YPrPb             ()
    {0x1802020E, {118, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_NTSC_YPrPb_16x9        ()
    {0x48040415, {118, 36,  720,  480, VIC_17_576p_50__4_3}}, // 720x480_PAL_I_YPrPb            ()
    {0x58040415, {118, 36,  720,  480, VIC_18_576p_50_16_9}}, // 720x480_PAL_I_YPrPb_16x9       ()
    {0x0802021A, {118, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_PAL_60_YPrPb           ()
    {0x1802021A, {118, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_PAL_60_YPrPb_16x9      ()
    {0x88080801, {119, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_480P                   ()

    {0x04020202, { 95, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_TO_NTSC_M_YC
    {0x14020202, { 95, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_TO_NTSC_M_YC_16x9
    {0x20020202, { 95, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_TO_NTSC_M_RGB
    {0x30020202, { 95, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_TO_NTSC_M_RGB_16x9

    // 720x576
    {0x48060617, {118, 25,  720,  576, VIC_17_576p_50__4_3}}, // 720x576_PAL_I_YPrPb            ()
    {0x58060617, {118, 25,  720,  576, VIC_18_576p_50_16_9}}, // 720x576_PAL_I_YPrPb_16x9       ()

    // 720p
    {0x880B0A02, {299, 25, 1280,  720, VIC_04_720p_60_16_9}}, // 1280x720P_720P                 ()

    // 1080i
    {0x880E0C03, {232, 22, 1920,  540, VIC_05_1080i_60_16_9}}, // 1920x1080I_1080I              ()

    // New modes UNK
    {0x00020072, { 95, 37,  640,  480, VIC_02_480p_60__4_3}},

    // End of table / AV Off
    {0x00000000, {  0,  0,    0,    0, VIC_00_VIC_Unavailable}},
};

const bios_mode_sync video_sync_settings_focus_bios[XBOX_VIDEO_BIOS_MODE_SYNC_COUNT] = {
    {0x0802020E, {62, 16,  6,  9, 0}},
    {0x880E0C03, {44, 88,  2,  5, 0}}, // 1920x1080I_1080I              ()

    // End of table / AV Off
    {0x00000000, { 0,  0,  0,  0, 0}},
};

const bios_mode video_settings_xcalibur_bios[XBOX_VIDEO_BIOS_MODE_COUNT] = {
    // 640x480
    {0x0801010D, { 95, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_NTSC_YPrPb             ()
    {0x1801010D, { 95, 37,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_NTSC_YPrPb_16x9        ()
    {0x080F0D12, {136, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_NTSC_YPrPb        ()
    {0x180F0D12, {136, 37,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_FPAR_NTSC_YPrPb_16x9   ()
    {0x48030314, { 95, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_PAL_I_YPrPb            (Tested)
    {0x58030314, { 95, 37,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_PAL_I_YPrPb_16x9       ()
    {0x48100E18, { 95, 37,  640,  480, VIC_17_576p_50__4_3}}, // 640x480_FPAR_PAL_I_YPrPb       ()
    {0x58100E18, { 95, 37,  640,  480, VIC_18_576p_50_16_9}}, // 640x480_FPAR_PAL_I_YPrPb_16x9  ()
    {0x08010119, { 95, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_PAL_60_YPrPb           (Tested)
    {0x18010119, { 95, 37,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_PAL_60_YPrPb_16x9      (Tested)
    {0x080F0D1B, { 95, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_PAL_60_YPrPb      ()
    {0x180F0D1B, { 95, 37,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_FPAR_PAL_60_YPrPb_16x9 ()
    {0x88070701, { 95, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_480P                   (Tested)
    {0x88110F01, { 95, 37,  720,  480, VIC_02_480p_60__4_3}}, // 640x480_FPAR_480P              (Tested, pillar boxed)

    {0x04010101, { 95, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_TO_NTSC_M_YC
    {0x14010101, { 95, 37,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_TO_NTSC_M_YC_16x9
    {0x20010101, { 95, 37,  640,  480, VIC_02_480p_60__4_3}}, // 640x480_TO_NTSC_M_RGB
    {0x30010101, { 95, 37,  640,  480, VIC_03_480p_60_16_9}}, // 640x480_TO_NTSC_M_RGB_16x9

    // 640x576
    {0x48050516, {142, 41,  640,  576, VIC_17_576p_50__4_3}}, // 640x576_PAL_I_YPrPb            (Tested)
    {0x58050516, {142, 41,  640,  576, VIC_18_576p_50_16_9}}, // 640x576_PAL_I_YPrPb_16x9       (Tested)
    {0x4812101D, {130, 36,  640,  576, VIC_17_576p_50__4_3}}, // 640x576_FPAR_PAL_I_YPrPb       ()
    {0x5812101D, {130, 36,  640,  576, VIC_18_576p_50_16_9}}, // 640x576_FPAR_PAL_I_YPrPb_16x9  ()

    // 720x480
    {0x88080801, { 95, 37,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_480P                   (Tested)
    {0x0802020E, { 95, 37,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_NTSC_YPrPb             ()
    {0x1802020E, { 95, 37,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_NTSC_YPrPb_16x9        ()
    {0x48040415, { 95, 25,  720,  480, VIC_17_576p_50__4_3}}, // 720x480_PAL_I_YPrPb            ()
    {0x58040415, { 95, 25,  720,  480, VIC_18_576p_50_16_9}}, // 720x480_PAL_I_YPrPb_16x9       ()
    {0x0802021A, { 95, 37,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_PAL_60_YPrPb           (Tested)
    {0x1802021A, { 95, 37,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_PAL_60_YPrPb_16x9      (Tested)

    {0x04020202, { 95, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_TO_NTSC_M_YC
    {0x14020202, { 95, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_TO_NTSC_M_YC_16x9
    {0x20020202, { 95, 36,  720,  480, VIC_02_480p_60__4_3}}, // 720x480_TO_NTSC_M_RGB
    {0x30020202, { 95, 36,  720,  480, VIC_03_480p_60_16_9}}, // 720x480_TO_NTSC_M_RGB_16x9

    // 720x576
    {0x48060617, {137, 42,  720,  576, VIC_17_576p_50__4_3}}, // 720x576_PAL_I_YPrPb            (Tested)
    {0x58060617, {137, 42,  720,  576, VIC_18_576p_50_16_9}}, // 720x576_PAL_I_YPrPb_16x9       (Tested)

    // 720p
    {0x880B0A02, {259, 25, 1280,  720, VIC_04_720p_60_16_9}}, // 1280x720P_720P                 (Tested)

    // 1080i
    {0x880E0C03, {186, 22, 1920,  540, VIC_05_1080i_60_16_9}}, // 1920x1080I_1080I              (Tested, seems to require custom timings)

    // New modes UNK
    {0x00020072, { 95, 37,  640,  480, VIC_02_480p_60__4_3}},

    // End of table / AV Off
    {0x00000000, {  0,  0,    0,    0, VIC_00_VIC_Unavailable}},
};

const bios_mode_sync video_sync_settings_xcalibur_bios[XBOX_VIDEO_BIOS_MODE_SYNC_COUNT] = {
    {0x0802020E, {62, 16,  6,  9, 0}},
    {0x880E0C03, {44, 88,  2,  5, 0}}, // CEA-861 1920x1080I_1080I              (Tested)

    // End of table / AV Off
    {0x00000000, { 0,  0,  0,  0, 0}},
};

// VIC mode values
typedef enum {
    XBOX_VIDEO_VGA,
    XBOX_VIDEO_480p_640,
    XBOX_VIDEO_480p_720,
    XBOX_VIDEO_720p,
    XBOX_VIDEO_1080i
} video_modes_vic;

const video_setting video_settings_conexant[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3}, // VGA
    {118, 36,  640, 480, VIC_02_480p_60__4_3},    // 480p
    {118, 36,  720, 480, VIC_03_480p_60_16_9},    // 480p (wide)
    {299, 25, 1280, 720, VIC_04_720p_60_16_9},    // 720p
    {233, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

const video_setting video_settings_focus[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3}, // VGA
    {118, 36,  640, 480, VIC_02_480p_60__4_3},    // 480p
    {118, 36,  720, 480, VIC_03_480p_60_16_9},    // 480p (wide)
    {299, 25, 1280, 720, VIC_04_720p_60_16_9},    // 720p
    {233, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

const video_setting video_settings_xcalibur[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3}, // VGA
    { 96, 36,  640, 480, VIC_02_480p_60__4_3},    // 480p
    { 96, 36,  720, 480, VIC_03_480p_60_16_9},    // 480p (wide)
    {259, 25, 1280, 720, VIC_04_720p_60_16_9},    // 720p
    {185, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

#endif // __XBOX_H__
