#ifndef __ADV7511_I2C_H__
#define __ADV7511_I2C_H__

#include "stm32.h"

void adv7511_i2c_init();
I2C_HandleTypeDef* adv7511_i2c_instance();

#endif // __ADV7511_I2C_H__
