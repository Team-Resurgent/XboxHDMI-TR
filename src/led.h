#ifndef __LED_H__
#define __LED_H__

#include "stm32.h"

#include <stdbool.h>

void led_init();
void led_status1(bool state);
void led_status2(bool state);

#endif // __LED_H__
