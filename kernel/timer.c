#include "kernel.h"

#define CLOCK_FREQ 12500000
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
void time_intr_switch(int on) {
    usize sie; asm volatile("csrr %0, sie":"=r"(sie));
    if (on) sie |= (1 << 5); else sie &= ~(1 << 5);
    asm volatile("csrw sie, %0"::"r"(sie));
}
