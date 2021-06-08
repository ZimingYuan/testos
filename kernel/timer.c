#include "kernel.h"

#ifdef K210
#define CLOCK_FREQ (403000000 / 62)
#else
#define CLOCK_FREQ 12500000
#endif

#define TICKS_PER_SEC 100
#define MSEC_PER_SEC 1000

usize get_time() {
    usize timer; asm volatile("rdtime %0":"=r"(timer)); return timer;
}
void set_next_trigger() {
    set_timer(get_time() + CLOCK_FREQ / TICKS_PER_SEC);
}
usize get_time_ms() {
    return get_time() / (CLOCK_FREQ / MSEC_PER_SEC);
}
