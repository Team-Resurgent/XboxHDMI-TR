#ifndef __XBOX_H__
#define __XBOX_H__

enum xbox_encoder {
    ENCODER_CONEXANT,
    ENCODER_FOCUS,
    ENCODER_XCALIBUR
};

typedef struct video_setting {
    uint16_t hs_delay;
    uint16_t vs_delay;
    uint16_t active_w;
    uint16_t active_h;
} video_setting;

// https://github.com/XboxDev/nxdk/blob/master/lib/hal/video.c

typedef enum {
    VIDEO_REGION_NTSCM,
    VIDEO_REGION_NTSCJ,
    VIDEO_REGION_PAL
} video_regions;

typedef enum {
    XBOX_VIDEO_VGA,
    XBOX_VIDEO_480p_640,
    XBOX_VIDEO_480p_720,
    XBOX_VIDEO_720p,
    XBOX_VIDEO_1080i
} video_modes;

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

#endif
