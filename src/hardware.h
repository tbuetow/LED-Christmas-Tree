#pragma once
#include <stdint.h>

extern volatile unsigned long tick_10ms;

void init_CPU();
void init_gpio();
void init_timer();
