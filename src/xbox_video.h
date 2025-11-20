#ifndef __XBOX_H__
#define __XBOX_H__

typedef enum xbox_encoder {
    ENCODER_CONEXANT = 0x8A,
    ENCODER_FOCUS = 0xD4,
    ENCODER_XCALIBUR = 0xE0
} xbox_encoder;

typedef struct video_setting {
    uint16_t delay_hs;
    uint16_t delay_vs;
    uint16_t active_w;
    uint16_t active_h;
} video_setting;

// https://github.com/XboxDev/nxdk/blob/master/lib/hal/video.c

typedef enum {
    VIDEO_REGION_NTSCM = 0x00000100,
    VIDEO_REGION_NTSCJ = 0x00000200,
    VIDEO_REGION_PAL_I = 0x00000300,
    VIDEO_REGION_PAL_M = 0x00000400
} video_region;

typedef enum {
    // NTSC North America
    VIDEO_NTSCM_640x480i_NTSCM_60Hz,
    VIDEO_NTSCM_640x480p_NTSCM_60Hz,
    VIDEO_NTSCM_720x480i_NTSCM_60Hz,
    VIDEO_NTSCM_720x480p_NTSCM_60Hz,
    VIDEO_NTSCM_1280x720p_NTSCM_60Hz,
    VIDEO_NTSCM_1920x1080i_NTSCM_60Hz,
    // NTSC Japan
    VIDEO_NTSCJ_640x480i_NTSCJ_60Hz,
    VIDEO_NTSCJ_720x480i_NTSCJ_60Hz,
    VIDEO_NTSCJ_640x480p_NTSCJ_60Hz,
    VIDEO_NTSCJ_720x480p_NTSCJ_60Hz,
    VIDEO_NTSCJ_1280x720p_NTSCJ_60Hz,
    VIDEO_NTSCJ_1920x1080i_NTSCJ_60Hz,
    // PAL modes
    VIDEO_PAL_640x480_50Hz,
    VIDEO_PAL_720x480_50Hz,
    VIDEO_PAL_640x480_60Hz,
    VIDEO_PAL_720x480_60Hz,
    // VGA
    VIDEO_VGA_640x480_SVGA_60Hz,
    VIDEO_VGA_800x600_SVGA_60Hz,
    VIDEO_VGA_1024x768_SVGA_60Hz
} video_modes;

typedef enum {
    XBOX_VIDEO_VGA,
    XBOX_VIDEO_480p_640,
    XBOX_VIDEO_480p_720,
    XBOX_VIDEO_720p,
    XBOX_VIDEO_1080i
} video_modes_vic;

video_setting video_settings_conexant[] = {
    {119, 36,  640, 480}, // VGA
    {118, 36,  640, 480}, // 480p
    {118, 36,  720, 480}, // 480p (wide)
    {299, 25, 1280, 720}, // 720p
    {233, 22, 1920, 540}  // 1080i
};

video_setting video_settings_focus[] = {
    {119, 36,  640, 480}, // VGA
    {118, 36,  640, 480}, // 480p
    {118, 36,  720, 480}, // 480p (wide)
    {299, 25, 1280, 720}, // 720p
    {233, 22, 1920, 540}  // 1080i
};

video_setting video_settings_xcalibur[] = {
    {119, 36,  640, 480}, // VGA
    { 96, 36,  640, 480}, // 480p
    {128, 36,  720, 480}, // 480p (wide)
    {259, 25, 1280, 720}, // 720p
    {185, 22, 1920, 540}  // 1080i
};

#endif // __XBOX_H__
