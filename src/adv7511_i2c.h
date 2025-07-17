#pragma once 

#include "stm32.h"

void adv7511_i2c_init();
I2C_HandleTypeDef* adv7511_i2c_instance();