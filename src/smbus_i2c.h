#ifndef __SMBUS_I2C_H__
#define __SMBUS_I2C_H__

#include <stdbool.h>
#include <stdint.h>

#pragma pack(1)
typedef struct
{
    uint8_t encoder;
    uint8_t region;
    uint32_t mode;
    uint32_t titleid;
} SMBusSettings;
#pragma pack()

void smbus_i2c_init();

bool video_mode_updated();

void ack_video_mode_update();

const SMBusSettings * const getSMBusSettings();

bool bios_tookover();

#endif // __SMBUS_I2C_H__