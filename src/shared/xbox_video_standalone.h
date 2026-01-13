#ifndef __XBOX_VIDEO_STANDALONE_H__
#define __XBOX_VIDEO_STANDALONE_H__

#include <stdint.h>
#include <stdbool.h>
#include "adv7511_minimal.h"
#include "adv7511_vic.h"
#include "types.h"

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

void stand_alone_loop(adv7511 * encoder, const xbox_encoder xb_encoder);

void set_video_mode_vic(const xbox_encoder xb_encoder, const uint8_t mode, const bool wide, const bool interlaced);

#endif // __XBOX_VIDEO_STANDALONE_H__
