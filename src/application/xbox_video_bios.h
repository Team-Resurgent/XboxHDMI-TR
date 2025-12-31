#ifndef __XBOX_VIDEO_BIOS_H__
#define __XBOX_VIDEO_BIOS_H__

#include <stdint.h>

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

// Mode
#define XBOX_VIDEO_MODE_BIT_WIDESCREEN 0x10000000
#define XBOX_VIDEO_MODE_BIT_SCART      0x20000000 // RGB
#define XBOX_VIDEO_MODE_BIT_MASK       0xC0000000
#define XBOX_VIDEO_MODE_BIT_480SDTV    0x00000000
#define XBOX_VIDEO_MODE_BIT_576SDTV    0x40000000
#define XBOX_VIDEO_MODE_BIT_HDTV       0x80000000
#define XBOX_VIDEO_MODE_BIT_VGA        0xC0000000
#define XBOX_VIDEO_DAC_MASK            0x0F000000
#define XBOX_VIDEO_MODE_MASK           0x0000FFFF
// Avinfo
#define XBOX_AVINFO_INTERLACED         0x00200000
#define XBOX_AVINFO_FILED              0x01000000

#pragma pack(1)
typedef struct {
    uint16_t hs_delay;          // Increase value to push picture left
    uint16_t vs_delay;          // Increase value to push picture up
    uint16_t h_active;          // Horizontal resolution
    uint16_t v_active;          // Vertical resolution
    uint16_t hsync_placement;   // H Sync
    uint16_t hsync_duration;    // H Front Porch
    uint16_t vsync_placement;   // V Sync
    uint16_t vsync_duration;    // V Front Porch
    uint8_t  interlaced_offset; // 3 bit
} VideoMode;
#pragma pack()

static const VideoMode XCALIBUR_TABLE[] = {
    { 95, 37,  640,  480,  43,   2,  7,  2, 0}, // 01 640x480_NTSC_RGB        OK
    { 95, 37,  720,  480,  41,   6,  7,  6, 0}, // 02 720x480_NTSC_RGB        OK
    { 95, 38,  640,  480,  63,  24,  1, 10, 0}, // 03 640x480_PAL_RGB         OK
    {137, 38,  720,  480,  41,  50,  1, 10, 0}, // 04 720x480_PAL_RGB         OK
    {142, 41,  640,  576, 127,  47,  6,  6, 0}, // 05 640x576_PAL_RGB         OK
    {137, 42,  720,  576,   5,  45,  6,  6, 0}, // 06 720x576_PAL_RGB         OK
    { 95, 36,  640,  480,  43,   2,  8,  5, 0}, // 07 640x480_480P            OK
    { 95, 36,  720,  480,  41,   6,  8,  5, 0}, // 08 720x480_480P            OK
    {300, 25,  960,  720,  69,  80,  4,  5, 0}, // 09 960x720                 ?
    {259, 25, 1280,  720, 110,  40,  5,  5, 0}, // 0A 720p                    ?
    {259, 25, 1280,  720, 110,  40,  5,  5, 0}, // 0B 720p_60                 OK
    {236, 40, 1440, 1080,  43,  88,  4, 10, 0}, // 0C 1440x1080               ?
    {236, 40, 1920, 1080,  43,  88,  4, 10, 0}, // 0D 1080                    ?
    {187, 41, 1920, 1080,  92,  40,  3, 10, 0}, // 0E 1080i                   Untested
    {136, 37,  640,  480,  81,   1,  7,  6, 0}, // 0F 640x480_FPAR_NTSC_RGB   OK
    {177, 38,  640,  480,  81,  42,  1, 10, 0}, // 10 640x480_FPAR_PAL_RGB    OK
    {127, 36,  654,  480,  41,  37,  8,  6, 0}, // 11 640x480_FPAR_480P       OK
    {142, 41,  640,  576,  87,   7,  6,  6, 0}  // 12 640x576_FPAR_PAL_RGB    OK
};

static const VideoMode CONEXANT_TABLE[] = {
    {122, 34,  640,  480,  13,  32, 10,  3, 0}, // 01 640x480_NTSC_RGB        OK
    {134, 34,  720,  480,  15,  32, 10,  3, 0}, // 02 720x480_NTSC_RGB        OK
    {254, 36,  640,  480,  55,  32,  8,  3, 0}, // 03 640x480_PAL_RGB         OK
    {270, 36,  720,  480,  59,  32,  8,  3, 0}, // 04 720x480_PAL_RGB         OK
    {132, 39,  640,  576,  15,  32,  9,  3, 0}, // 05 640x576_PAL_RGB         Cropped vertically
    {148, 39,  720,  576,  17,  33,  9,  3, 0}, // 06 720x576_PAL_RGB         Cropped vertically
    {119, 36,  720,  480,  17,  63,  8,  6, 0}, // 07 640x480_480P            OK
    {119, 36,  720,  480,  17,  63,  8,  6, 0}, // 08 720x480_480P            OK
    {300, 25,  960,  720,  69,  80,  4,  5, 0}, // 09 960x720                 ?
    {299, 25, 1280,  720,  69,  80,  4,  5, 0}, // 0A 720p                    ?
    {299, 25, 1280,  720,  69,  80,  4,  5, 0}, // 0B 720p_60                 OK
    {236, 40, 1440, 1080,  43,  88,  4, 10, 0}, // 0C 1440x1080               ?
    {236, 40, 1920, 1080,  43,  88,  4, 10, 0}, // 0D 1080                    ?
    {235, 41, 1920, 1080,  44,  88,  3, 10, 0}, // 0E 1080i                   Untested
    {165, 34,  640,  480,  48,  32, 10,  3, 0}, // 0F 640x480_FPAR_NTSC_RGB   OK
    {318, 34,  640,  480,  91,  32, 10,  3, 0}, // 10 640x480_FPAR_PAL_RGB    OK
    {153, 36,  654,  480,  15,  63,  8,  6, 0}, // 11 640x480_FPAR_480P       OK
    {179, 39,  640,  576,  48,  32,  9,  3, 0}  // 12 640x576_FPAR_PAL_RGB    Cropped vertically
};

static const VideoMode FOCUS_TABLE[] = {
    {180, 26,  640,  480, 115, 64, 18,  2, 0}, // 01 640x480_NTSC_RGB        ?
    {140, 26,  720,  480,  75, 64, 18,  2, 0}, // 02 720x480_NTSC_RGB        ?
    {144, 24,  640,  480,  79, 64, 20,  2, 0}, // 03 640x480_PAL_RGB         ?
    { 84, 24,  720,  480,  59, 64, 20,  2, 0}, // 04 720x480_PAL_RGB         ?
    {144, 26,  640,  576,  79, 64, 22,  2, 0}, // 05 640x576_PAL_RGB         ?
    { 84, 26,  720,  576,  59, 64, 22,  2, 0}, // 06 720x576_PAL_RGB         ?
    {120, 36,  720,  480,  17, 63,  8,  6, 0}, // 07 640x480_480P            ?
    {120, 36,  720,  480,  17, 63,  8,  6, 0}, // 08 720x480_480P            ?
    {300, 25,  960,  720,  69, 80,  4,  5, 0}, // 09 960x720                 ?
    {300, 25, 1280,  720,  69, 80,  4,  5, 0}, // 0A 720p                    ?
    {300, 25, 1280,  720,  69, 80,  4,  5, 0}, // 0B 720p_60                 ?
    {236, 40, 1440, 1080,  43, 88,  4, 10, 0}, // 0C 1440x1080               ?
    {236, 40, 1920, 1080,  43, 88,  4, 10, 0}, // 0D 1080                    ?
    {235, 41, 1920, 1080,  44, 88,  3, 10, 0}, // 0E 1080i                   ?
    {180, 26,  640,  480, 115, 64, 18,  2, 0}, // 0F 640x480_FPAR_NTSC_RGB   ?
    { 44, 24,  640,  480,  79, 64, 20,  2, 0}, // 10 640x480_FPAR_PAL_RGB    ?
    {153, 36,  655,  480,  15, 63,  8,  6, 0}, // 11 640x480_FPAR_480P       OK
    {144, 26,  640,  576,  79, 64, 22,  2, 0}  // 12 640x576_FPAR_PAL_RGB    ?
};

#endif // __XBOX_VIDEO_BIOS_H__
