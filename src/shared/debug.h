#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "stm32.h"

void debug_init();
void debug_log(const char *fmt, ...);
void debug_ring_log(const char *fmt, ...);
void debug_ring_flush();

#endif // __DEBUG_H__
