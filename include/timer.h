#ifndef __TIMER_H
#define __TIMER_H
#include <stdint.h>

void init_timer(uint32_t frequency);

uint32_t get_timer_ticks();

#endif
