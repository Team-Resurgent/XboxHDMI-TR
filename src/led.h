#ifndef __LED_H__
#define __LED_H__

#include "stm32.h"

#include <stdbool.h>

void led_init();
void set_led_1(bool state); // Green led for the HD+
void set_led_2(bool state); // Blue led for the HD+

#endif // __LED_H__
