#ifndef __GPIO_H__
#define __GPIO_H__

#include "stdbool.h"

void init_gpio();

void set_led_1(bool state); // Green led for the HD+
void set_led_2(bool state); // Blue led for the HD+
bool recovery_jumper_enabled();

#endif // __GPIO_H__
