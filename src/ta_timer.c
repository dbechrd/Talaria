#include "ta_timer.h"
#include "dlb_types.h"
#include "SDL/SDL_timer.h"

static u64 perf_frequency;
static double perf_frequency_ms;
static double perf_frequency_us;
static u64 perf_epoch;
static u64 perf_timers[16];

void ta_timer_init()
{
	// 10,000,000 (per second)
	perf_frequency = SDL_GetPerformanceFrequency();
	// 10,000 (per millisecond)
    perf_frequency_ms = perf_frequency / 1000.0;
    perf_frequency_us = perf_frequency_ms / 1000.0;
	perf_epoch = SDL_GetPerformanceCounter();
}

u64 ta_timer_elapsed_ticks()
{
	u64 now = SDL_GetPerformanceCounter();
	u64 elapsed_ticks = now - perf_epoch;
	return elapsed_ticks;
}

double ta_timer_elapsed_ms()
{
	u64 elapsed_ticks = ta_timer_elapsed_ticks();
    double elapsed_ms = elapsed_ticks / perf_frequency_ms;
	return elapsed_ms;
}

double ta_timer_elapsed_us()
{
    u64 elapsed_ticks = ta_timer_elapsed_ticks();
    double elapsed_us = elapsed_ticks / perf_frequency_us;
    return elapsed_us;
}

double ta_timer_elapsed_sec()
{
	double elapsed_ms = ta_timer_elapsed_ms();
	double elapsed_sec = elapsed_ms / 1000;
	return elapsed_sec;
}

// Number of milliseconds since last second (modulo)
u64 ta_timer_only_ms()
{
    u64 elapsed_ticks = ta_timer_elapsed_ticks();
    u64 ticks_per_ms = perf_frequency / 1000;
    u64 now_ms = elapsed_ticks % ticks_per_ms;
	return now_ms;
}