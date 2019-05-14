#include "ta_timer.h"
#include "dlb_types.h"
#include "SDL/SDL_timer.h"

static u64 perf_frequency;
static u64 perf_frequency_ms;
static u64 perf_epoch;
static u64 perf_timers[16];

void ta_timer_init()
{
	// 10,000,000 (per second)
	perf_frequency = SDL_GetPerformanceFrequency();
	// 10,000 (per millisecond)
	perf_frequency_ms = perf_frequency / 1000;
	perf_epoch = SDL_GetPerformanceCounter();
}

u64 ta_timer_elapsed_ticks()
{
	u64 now = SDL_GetPerformanceCounter();
	u64 elapsed_ticks = now - perf_epoch;
	return elapsed_ticks;
}

u64 ta_timer_elapsed_ms()
{
	u64 elapsed_ticks = ta_timer_elapsed_ticks();
	u64 elapsed_ms = elapsed_ticks / perf_frequency_ms;
	return elapsed_ms;
}

double ta_timer_elapsed_sec()
{
	u64 elapsed_ms = ta_timer_elapsed_ms();
	double elapsed_sec = elapsed_ms / 1000.0;
	return elapsed_sec;
}

// Number of milliseconds since last second (modulo)
u64 ta_timer_only_ms()
{
	u64 now_ms = ta_timer_elapsed_ms() % 1000;
	return now_ms;
}