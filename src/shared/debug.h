#pragma once

void debug_init();
void debug_log(const char *fmt, ...);
void debug_ring_log(const char *fmt, ...);
void debug_ring_flush();
