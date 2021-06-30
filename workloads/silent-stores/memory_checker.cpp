/* push this through gem5 to check memory values are being written correctly */
#include<assert.h>

#ifdef DEBUG
#include<iostream>
// yeah, you have to keep calling it with \0; but it allows you to concatenate
// in prints without littering ifdef debug everywhere *shrug*
#define DPRINT(x) if (x == '\0') std::cout << std::endl << "[[DEBUG]] "; else std::cout << (x);
#define DASSERT(x) assert(x);
#else
#define DPRINT(x) do {} while(0)
#define DASSERT(x) do {} while(0)
#endif

#ifdef GEM5
#ifdef DEBUG
#error do not use debug flag with gem5 builds
#endif
#include "gem5/m5ops.h"
#define toggle_m5_region() m5_work_begin(0, 0); m5_dump_reset_stats(0, 0);
#else
#define toggle_m5_region() do {} while(0)
#endif

#define MSG_LEN 256

void fill_mem(volatile char* mem) {
    volatile char* val = mem;
    volatile char* end = mem+MSG_LEN;
    while (val != end) {
        for (char cval = 'A'; cval <= 'Z' && val!=end; cval++) {
            *val++ = cval;
        }
        for (char cval = 'a'; cval <= 'z' && val!=end; cval++) {
            *val++ = cval;
        }
    }
}

void fill_mem_rev(volatile char* mem) {
    volatile char* val = mem;
    volatile char* end = mem+MSG_LEN;
    while (val != end) {
        for (char cval = 'a'; cval <= 'z' && val!=end; cval++) {
            *val++ = cval;
        }
        for (char cval = 'A'; cval <= 'Z' && val!=end; cval++) {
            *val++ = cval;
        }
    }
}

#ifdef DEBUG
void print_mem(volatile char* mem) {
    for (auto val=mem, end=mem+MSG_LEN; val!=end; val++)
        DPRINT(*val);
    DPRINT('\0');
}
#endif

void test_mem(volatile char* mem) {
    volatile char* val = mem;
    volatile char* end = mem+MSG_LEN;
    while (val != end) {
        for (char cval = 'A'; cval <= 'Z' && val!=end; cval++) {
            DPRINT(*val);
            DPRINT(':');
            DPRINT(cval);
            DPRINT(' ');
            assert(*val++ == cval);
        }
        for (char cval = 'a'; cval <= 'z' && val!=end; cval++) {
            DPRINT(*val);
            DPRINT(':');
            DPRINT(cval);
            DPRINT(' ');
            assert(*val++ == cval);
        }
    }
}

void test_mem_rev(volatile char* mem) {
    volatile char* val = mem;
    volatile char* end = mem+MSG_LEN;
    while (val != end) {
        for (char cval = 'a'; cval <= 'z' && val!=end; cval++) {
            DPRINT(*val);
            DPRINT(':');
            DPRINT(cval);
            DPRINT(' ');
            assert(*val++ == cval);
        }
        for (char cval = 'A'; cval <= 'Z' && val!=end; cval++) {
            DPRINT(*val);
            DPRINT(':');
            DPRINT(cval);
            DPRINT(' ');
            assert(*val++ == cval);
        }
    }
    DPRINT('\0');
}

int main() {
    DPRINT('\0');
    volatile char mem[MSG_LEN];

    fill_mem(mem);
    DPRINT('\0');
#ifdef DEBUG
    print_mem(mem);
#endif
    test_mem(mem);

    fill_mem(mem);
    test_mem(mem);

    fill_mem_rev(mem);
    DPRINT('\0');
#ifdef DEBUG
    print_mem(mem);
#endif
    test_mem_rev(mem);

    DPRINT("ret0\n");
    return 0;
}
