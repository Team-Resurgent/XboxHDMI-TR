#ifndef __XBOX_VIDEO_STANDALONE_H__
#define __XBOX_VIDEO_STANDALONE_H__

#include <stdint.h>
#include "adv7511.h"

#pragma pack(1)
typedef struct video_setting {
    uint16_t delay_hs;
    uint16_t delay_vs;
    uint16_t active_w;
    uint16_t active_h;
    uint8_t  vic;
} video_setting_vic;
#pragma pack()

typedef enum {
    XBOX_VIDEO_VGA,
    XBOX_VIDEO_480p_640,
    XBOX_VIDEO_480p_720,
    XBOX_VIDEO_720p,
    XBOX_VIDEO_1080i
} video_modes_vic;

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

#endif // __XBOX_VIDEO_VIC_H__
