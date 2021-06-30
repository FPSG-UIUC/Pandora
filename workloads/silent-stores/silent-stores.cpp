#include <iostream>
#ifdef GEM5
#ifdef DEBUG
#error do not use debug flag with gem5 builds
#endif
#include "gem5/m5ops.h"
#define toggle_m5_region() m5_work_begin(0, 0); m5_dump_reset_stats(0, 0);
#else
#define toggle_m5_region() do {} while(0)
#endif

#define ARR_SIZE 512

int main() {
    // setup loads
    uint16_t load_target[ARR_SIZE];
    uint16_t* end=load_target+ARR_SIZE;
    for (uint16_t* buf=load_target; buf < end; buf++) {
        *buf = 0;
    }

    // do many loads
    volatile uint16_t tmp;
    /* gem5 ROI start */
    toggle_m5_region();

    for (uint16_t* buf=load_target; buf < end; buf++) {
        tmp = *buf;
    }

    /* gem5 ROI end */
    toggle_m5_region();

    return 0;
}
