#pragma once
#include "dlb/dlb_types.h"

void ta_timer_init();
u64 ta_timer_elapsed_ticks();
double ta_timer_elapsed_ms();
double ta_timer_elapsed_sec();
u64 ta_timer_only_ms();