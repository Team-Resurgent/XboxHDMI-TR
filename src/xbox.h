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

#define XBOX_VIDEO_VGA   0
#define XBOX_VIDEO_480p  1
#define XBOX_VIDEO_720p  2
#define XBOX_VIDEO_1080i 3

video_setting video_settings_conexant[] = {
    {119, 36,  720, 480}, // VGA
    {118, 36,  720, 480}, // 480p
    {299, 25, 1280, 720}, // 720p
    {233, 22, 1920, 540}  // 1080i
};

video_setting video_settings_focus[] = {
    {119, 36,  720, 480}, // VGA
    {118, 36,  720, 480}, // 480p
    {299, 25, 1280, 720}, // 720p
    {233, 22, 1920, 540}  // 1080i
};

video_setting video_settings_xcalibur[] = {
    {119, 36,  720, 480}, // VGA
    {118, 36,  720, 480}, // 480p
    {259, 25, 1280, 720}, // 720p
    {185, 22, 1920, 540}  // 1080i
};

#endif
