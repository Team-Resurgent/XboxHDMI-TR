#ifndef __ADV7511_XBOX_H__
#define __ADV7511_XBOX_H__

// XBOX / TR HDMI specific ADV functions

#include "adv7511_i2c.h"
#include "adv7511_minimal.h"
#include "xbox_video_standalone.h"

void init_adv(adv7511 *encoder, const xbox_encoder xb_encoder);

void init_adv_encoder_specific(const xbox_encoder xb_encoder);

void init_adv_audio();

void update_avi_infoframe(const bool widescreen);

#endif // __ADV7511_XBOX_H__
