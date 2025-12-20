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
    uint16_t delay_hs;  // Increase value to push picture left
    uint16_t delay_vs;  // Increase value to push picture up
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

// Mode
#define XBOX_VIDEO_MODE_BIT_WIDESCREEN 0x10000000
#define XBOX_VIDEO_MODE_BIT_SCART      0x20000000 // RGB
// Avinfo
#define XBOX_AVINFO_INTERLACED         0x00200000

#define XBOX_VIDEO_MODE_BIT_MASK       0xC0000000
#define XBOX_VIDEO_MODE_BIT_480SDTV    0x00000000
#define XBOX_VIDEO_MODE_BIT_576SDTV    0x40000000
#define XBOX_VIDEO_MODE_BIT_HDTV       0x80000000
#define XBOX_VIDEO_MODE_BIT_VGA        0xC0000000

#define XBOX_VIDEO_DAC_MASK            0x0F000000
#define XBOX_VIDEO_MODE_MASK           0x0000FFFF

// VIC mode values
typedef enum {
    XBOX_VIDEO_VGA,
    XBOX_VIDEO_480p_640,
    XBOX_VIDEO_480p_720,
    XBOX_VIDEO_720p,
    XBOX_VIDEO_1080i
} video_modes_vic;

const video_setting video_settings_conexant[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3},
    {118, 36,  640, 480, VIC_02_480p_60__4_3},
    {118, 36,  720, 480, VIC_03_480p_60_16_9},
    {299, 25, 1280, 720, VIC_04_720p_60_16_9},
    {233, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

const video_setting video_settings_focus[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3},
    {118, 36,  640, 480, VIC_02_480p_60__4_3},
    {118, 36,  720, 480, VIC_03_480p_60_16_9},
    {299, 25, 1280, 720, VIC_04_720p_60_16_9},
    {233, 22, 1920, 540, VIC_05_1080i_60_16_9}
};

const video_setting video_settings_xcalibur[] = {
    {119, 36,  640, 480, VIC_01_VGA_640x480_4_3},
    { 96, 36,  640, 480, VIC_02_480p_60__4_3},
    { 96, 36,  720, 480, VIC_03_480p_60_16_9},
    {259, 25, 1280, 720, VIC_04_720p_60_16_9},
    {185, 22, 1920, 540, VIC_05_1080i_60_16_9}
};
// End VIC modes

typedef enum {
    CONEXANT = 0,
    FOCUS = 1,
    XCALIBUR = 2
} EncoderType;

typedef struct {
    uint16_t hs_delay;
    uint16_t vs_delay;
    uint16_t hactive;
    uint16_t vactive;
    int hsync_placement;
    int hsync_duration;
    int vsync_placement;
    int vsync_duration;
} VideoMode;

static const VideoMode XCALIBUR_TABLE[] = {
    { 96, 37,  640,  480,  43,   2,  7,  2}, // 01 640x480_NTSC_RGB
    { 96, 37,  720,  480,  41,   6,  7,  6}, // 02 720x480_NTSC_RGB
    { 96, 38,  640,  480,  63,  24,  1, 10}, // 03 640x480_PAL_RGB
    {138, 38,  720,  480,  41,  50,  1, 10}, // 04 720x480_PAL_RGB
    { 96, 42,  640,  576, 127, -47,  6,  6}, // 05 640x576_PAL_RGB
    {138, 42,  720,  576,   5,  45,  6,  6}, // 06 720x576_PAL_RGB
    { 96, 36,  640,  480,  43,   2,  8,  5}, // 07 640x480_480P
    { 96, 36,  720,  480,  41,   6,  8,  5}, // 08 720x480_480P
    {300, 25,  960,  720,  69,  80,  4,  5}, // 09 960x720 ?
    {260, 25, 1280,  720,  32,  40,  8,  6}, // 0A 720p ?
    {260, 25, 1280,  720,  32,  40,  8,  6}, // 0B 720p_60
    {236, 40, 1440, 1080,  43,  88,  4, 10}, // 0C 1440x1080 ?
    {236, 40, 1920, 1080,  43,  88,  4, 10}, // 0D 1080 ?
    {187, 41, 1920, 1080,  92,  40,  3, 10}, // 0E 1080i
    {136, 37,  640,  480,  81,  -1,  7,  6}, // 0F 640x480_FPAR_NTSC_RGB
    {178, 38,  640,  480,  81,  42,  1, 10}, // 10 640x480_FPAR_PAL_RGB
    { 96, 36,  654,  480,  41,  37,  8,  6}, // 11 640x480_FPAR_480P
    {136, 42,  640,  576,  87,  -7,  6,  6}  // 12 640x576_FPAR_PAL_RGB
};

static const VideoMode CONEXANT_TABLE[] = {
    {122, 34,  640,  480,  13,  32, 10,  3}, // 640x480_NTSC_RGB
    {134, 34,  720,  480,  15,  32, 10,  3}, // 720x480_NTSC_RGB
    {254, 36,  640,  480,  55,  32,  8,  3}, // 640x480_PAL_RGB
    {270, 36,  720,  480,  59,  32,  8,  3}, // 720x480_PAL_RGB
    {132, 39,  640,  576,  15,  32,  9,  3}, // 640x576_PAL_RGB
    {148, 39,  720,  576,  17,  33,  9,  3}, // 720x576_PAL_RGB
    {120, 36,  720,  480,  17,  63,  8,  6}, // 640x480_480P
    {120, 36,  720,  480,  17,  63,  8,  6}, // 720x480_480P
    {300, 25,  960,  720,  69,  80,  4,  5}, // 960x720 ?
    {300, 25, 1280,  720,  69,  80,  4,  5}, // 720p ?
    {300, 25, 1280,  720,  69,  80,  4,  5}, // 720p_60
    {236, 40, 1440, 1080,  43,  88,  4, 10}, // 1440x1080 ?
    {236, 40, 1920, 1080,  43,  88,  4, 10}, // 1080 ?
    {235, 41, 1920, 1080,  44,  88,  3, 10}, // 1080i
    {165, 34,  640,  480,  48,  32, 10,  3}, // 640x480_FPAR_NTSC_RGB
    {318, 34,  640,  480,  91,  32, 10,  3}, // 640x480_FPAR_PAL_RGB
    {122, 36,  654,  480,  15,  63,  8,  6}, // 640x480_FPAR_480P
    {179, 39,  640,  576,  48,  32,  9,  3}  // 640x576_FPAR_PAL_RGB
};


static const VideoMode FOCUS_TABLE[] = {
    {180, 26,  640,  480, 115, 64, 18,  2}, // 640x480_NTSC_RGB
    {140, 26,  720,  480,  75, 64, 18,  2}, // 720x480_NTSC_RGB
    {144, 24,  640,  480,  79, 64, 20,  2}, // 640x480_PAL_RGB
    { 84, 24,  720,  480,  59, 64, 20,  2}, // 720x480_PAL_RGB
    {144, 26,  640,  576,  79, 64, 22,  2}, // 640x576_PAL_RGB
    { 84, 26,  720,  576,  59, 64, 22,  2}, // 720x576_PAL_RGB
    {120, 36,  720,  480,  17, 63,  8,  6}, // 640x480_480P
    {120, 36,  720,  480,  17, 63,  8,  6}, // 720x480_480P
    {300, 25,  960,  720,  69, 80,  4,  5}, // 960x720 ?
    {300, 25, 1280,  720,  69, 80,  4,  5}, // 720p ?
    {300, 25, 1280,  720,  69, 80,  4,  5}, // 720p_60
    {236, 40, 1440, 1080,  43, 88,  4, 10}, // 1440x1080 ?
    {236, 40, 1920, 1080,  43, 88,  4, 10}, // 1080 ?
    {235, 41, 1920, 1080,  44, 88,  3, 10}, // 1080i
    {180, 26,  640,  480, 115, 64, 18,  2}, // 640x480_FPAR_NTSC_RGB
    { 44, 24,  640,  480,  79, 64, 20,  2}, // 640x480_FPAR_PAL_RGB
    {122, 36,  654,  480,  15, 63,  8,  6}, // 640x480_FPAR_480P
    {144, 26,  640,  576,  79, 64, 22,  2}  // 640x576_FPAR_PAL_RGB
};


#endif // __XBOX_H__
