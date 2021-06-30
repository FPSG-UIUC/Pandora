/*********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "int32_sort.h"

#include <inttypes.h>
#include<x86intrin.h>

#ifdef DEBUG
#include<assert.h>
// yeah, you have to keep calling it with \0; but it allows you to concatenate
// in prints without littering ifdef debug everywhere *shrug*
#define DPRINT(x) printf(x);
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

#define NMAX 256
// #define NMAX 16384
#define NUM_TESTS 5
// #define NUM_TESTS 11
#define NUM_REPS 200

inline void clear_and_thrash(int32_t* buf) {
    int32_t* curr;

    // memcpy(buf, orig, NMAX * 4);

    curr = buf;
    for (int32_t* end_arr=buf+NMAX; curr!=end_arr; curr+=4) {
        _mm_clflush(curr);
    }
}

inline void thrash(unsigned char* buf, unsigned int dat) {
    unsigned char* curr;
    curr = buf;
    for (unsigned char* end_arr=buf+1024; curr!=end_arr; curr+=16) {
        memset(curr, dat, 16);
        _mm_clflush(curr);
    }
}

inline bool match_vals(int32_t* arr1, int32_t* arr2, unsigned len) {
    for (unsigned cc=0; cc<len; cc++) {
        if (arr1[cc] != arr2[cc]) {
            return false;
        }
    }
    return true;
}

void print_tandem(int32_t* arr1, int32_t* arr2, unsigned len) {
    int32_t* curr1 = arr1;
    int32_t* curr2 = arr2;
    for (int32_t* end_arr = arr1 + len; curr1 < end_arr; curr1+=16, curr2+=16) {
        printf("  %i--%i  ", curr1[ 0], curr2[ 0]);
        printf("  %i--%i  ", curr1[ 1], curr2[ 1]);
        printf("  %i--%i  ", curr1[ 2], curr2[ 2]);
        printf("  %i--%i\n", curr1[ 3], curr2[ 3]);
        printf("  %i--%i  ", curr1[ 4], curr2[ 4]);
        printf("  %i--%i  ", curr1[ 5], curr2[ 5]);
        printf("  %i--%i  ", curr1[ 6], curr2[ 6]);
        printf("  %i--%i\n", curr1[ 7], curr2[ 7]);
        printf("  %i--%i  ", curr1[ 8], curr2[ 8]);
        printf("  %i--%i  ", curr1[ 9], curr2[ 9]);
        printf("  %i--%i  ", curr1[10], curr2[10]);
        printf("  %i--%i\n", curr1[11], curr2[11]);
        printf("  %i--%i  ", curr1[12], curr2[12]);
        printf("  %i--%i  ", curr1[13], curr2[13]);
        printf("  %i--%i  ", curr1[14], curr2[14]);
        printf("  %i--%i",   curr1[15], curr2[15]);
        printf("\n");
    }
    printf("------\n");
}

void print_arr(int32_t* arr, unsigned len) {
    int32_t* curr = arr;
    for (int32_t* end_arr = arr + len; curr < end_arr; curr+=16) {
        printf("  %i  ", curr[ 0]);
        printf("  %i  ", curr[ 1]);
        printf("  %i  ", curr[ 2]);
        printf("  %i\n", curr[ 3]);
        printf("  %i  ", curr[ 4]);
        printf("  %i  ", curr[ 5]);
        printf("  %i  ", curr[ 6]);
        printf("  %i\n", curr[ 7]);
        printf("  %i  ", curr[ 8]);
        printf("  %i  ", curr[ 9]);
        printf("  %i  ", curr[10]);
        printf("  %i\n", curr[11]);
        printf("  %i  ", curr[12]);
        printf("  %i  ", curr[13]);
        printf("  %i  ", curr[14]);
        printf("  %i", curr[15]);
        printf("\n");
    }
    printf("------\n");
}

int main(void) {
    unsigned int garbage_val = 0;
    unsigned char* garbage = (unsigned char*)malloc(1024);  // for clearing the cache

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;
    uint64_t right_timing[NUM_REPS * NUM_TESTS];
    uint64_t wrong_timing[NUM_REPS * NUM_TESTS];

    unsigned right_run = 0, wrong_run = 0;
    unsigned guess_type = 0;

    printf("int32 implementation %s\n",int32_sort_implementation);
    printf("int32 version %s\n",int32_sort_version);
    printf("int32 compiler %s\n",int32_sort_compiler);

    toggle_m5_region();

    // unsigned sizes[NUM_TESTS] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    unsigned sizes[NUM_TESTS] = {NMAX/16, NMAX/8, NMAX/4, NMAX/2, NMAX};

    int32_t* v_arr = (int32_t*)aligned_alloc(128, NMAX * 4);
    int32_t* v_arr_orig = (int32_t*)aligned_alloc(128, NMAX * 4);
    int32_t* a_arr_c = (int32_t*)aligned_alloc(128, NMAX * 4);
    int32_t* a_arr_orig_c = (int32_t*)aligned_alloc(128, NMAX * 4);
    int32_t* a_arr_i = (int32_t*)aligned_alloc(128, NMAX * 4);
    int32_t* a_arr_orig_i = (int32_t*)aligned_alloc(128, NMAX * 4);

    // int32_t v_arr[NMAX],   v_arr_orig[NMAX];
    // int32_t a_arr_c[NMAX], a_arr_orig_c[NMAX];
    // int32_t a_arr_i[NMAX], a_arr_orig_i[NMAX];

    // printf("%x\n", v_arr);
    // printf("%x\n", a_arr_c);

    for (unsigned i=0; i<NMAX; ++i) {
        v_arr_orig[i] = random() % 64;
        a_arr_orig_c[i] = random() % 64;
        a_arr_orig_i[i] = random() % 64;
    }

    // print_tandem(v_arr, v_arr_orig, NMAX);

    memcpy(v_arr,   v_arr_orig, NMAX * 4);
    memcpy(a_arr_c, a_arr_orig_c, NMAX * 4);
    memcpy(a_arr_i, a_arr_orig_i, NMAX * 4);

    // print_tandem(v_arr, v_arr_orig, NMAX);
    // print_tandem(v_arr, a_arr_c, NMAX);

    memcpy(a_arr_c, v_arr, sizes[0] * 4);
    // if (!match_vals(a_arr_c, v_arr, sizes[0])) {
    //     return -1;
    // }

    // print_tandem(v_arr, a_arr_c, NMAX);

    // printf("%lu\n", sizeof(v_arr));
    // print_arr(v_arr, NMAX);
    // int32_sort(v_arr, NMAX/2);
    // print_arr(v_arr, NMAX);
    // memcpy(v_arr,   v_arr_orig, NMAX * 4);

    // memcpy(v_arr,   v_arr_orig, NMAX * 4);

    // print_arr(v_arr, NMAX);
    // int32_sort(v_arr, NMAX);
    // print_arr(v_arr, NMAX);

    // print_arr(a_arr_c, NMAX);
    // int32_sort(a_arr_c, NMAX);
    // print_arr(a_arr_c, 16);

    // print_arr(a_arr_i, 16);
    // int32_sort(a_arr_i, NMAX);
    // print_arr(a_arr_i, 16);

    int32_t* adv_selector[2] = {a_arr_c, a_arr_i};

    for (int j = 0; j < NUM_TESTS; j++) {
        for (int i = 0; i < NUM_REPS*2; i++) {

            /* flip back and forth on guess type */
            guess_type = guess_type ^ 0x1;
            /* victim access */
            // 1
            // 4, 7, 10, 13, 16 | 19,
            toggle_m5_region();
            int32_sort(v_arr, NMAX);
            toggle_m5_region();

            thrash(garbage, garbage_val++);

            /* adversary guess correct */
            // 2, 5, 8, 11, 14 | 17, 20
            toggle_m5_region();
            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            int32_sort(adv_selector[guess_type], NMAX);
            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                    "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            toggle_m5_region();
            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

            memcpy(v_arr, v_arr_orig, NMAX * 4);
            if(guess_type == 0){
                right_timing[right_run++] = end - start;  // victim time
                memcpy(a_arr_c, a_arr_orig_c, NMAX * 4);
                memcpy(a_arr_c, v_arr, sizes[j] * 4);
                clear_and_thrash(a_arr_c);
            }
            else{
                wrong_timing[wrong_run++] = end - start;  // victim time
                memcpy(a_arr_i, a_arr_orig_i, NMAX * 4);
                clear_and_thrash(a_arr_i);
            }

            /* toggle_m5_region(); */
            clear_and_thrash(v_arr);
        }
    }

#ifndef GEM5
    FILE* out_f = fopen("sort_timing_short.out", "w+");
#else
    FILE* out_f = fopen("sort_timing_short.gem5_out", "w+");
#endif

    fprintf(out_f, "sizes");
    for (unsigned j=0; j<NUM_TESTS; j++) {
        fprintf(out_f, ",%i", sizes[j]);
    }
    fprintf(out_f, "\n");

    uint64_t* timing_cur = right_timing;
    uint64_t* timing_end = right_timing + (NUM_REPS * NUM_TESTS);
    fprintf(out_f, "correct,%" PRIu64, *timing_cur++);
    do {
        fprintf(out_f, ",%" PRIu64, *timing_cur++);
    } while (timing_cur != timing_end);
    fprintf(out_f, "\n");

    timing_cur = wrong_timing;
    timing_end = wrong_timing + (NUM_REPS * NUM_TESTS);
    fprintf(out_f, "wrong,%" PRIu64, *timing_cur++);
    do {
        fprintf(out_f, ",%" PRIu64, *timing_cur++);
    } while (timing_cur != timing_end);
    fprintf(out_f, "\n");

    return 0;
}
