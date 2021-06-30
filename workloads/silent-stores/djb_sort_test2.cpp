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
#define DPRINT(x) if (x == '\0') std::cout << std::endl << "[[DEBUG]]"; else std::cout << (x);
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

#define MESSAGE_SIZE 256

inline void clear_and_thrash(unsigned char* buf) {
    unsigned char* curr;
    /* clear buffers */
    curr = buf;
    for (unsigned char* end=buf+MESSAGE_SIZE; curr!=end;
            curr+=16) {
        curr[ 0] = 0;
        curr[ 1] = 0;
        curr[ 2] = 0;
        curr[ 3] = 0;
        curr[ 4] = 0;
        curr[ 5] = 0;
        curr[ 6] = 0;
        curr[ 7] = 0;
        curr[ 8] = 0;
        curr[ 9] = 0;
        curr[10] = 0;
        curr[11] = 0;
        curr[12] = 0;
        curr[13] = 0;
        curr[14] = 0;
        curr[15] = 0;
        _mm_clflush(curr);
    }
}

inline void thrash(unsigned char* buf, unsigned int dat) {
    unsigned char* curr;
    curr = buf;
    for (unsigned char* end=buf+1024; curr!=end; curr+=16) {
        memset(curr, dat, 16);
    }
}

#define  NUM_REPS 120
int num_arrs = 16;
#define NUM_ELEMENTS 128


int main(void) {

    unsigned int garbage_val = 0;
    unsigned char* garbage = (unsigned char*)malloc(1024);  // for clearing the cache

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;
    uint64_t timings[NUM_REPS * num_arrs];

    int32_t* sort_arr = (int32_t*)aligned_alloc(128, NUM_ELEMENTS * sizeof(int32_t));

    int32_t** arrs = (int32_t**)malloc(num_arrs*sizeof(int32_t*));

    for(int i = 0; i < num_arrs; i++){
      arrs[i] = (int32_t*)malloc(NUM_ELEMENTS*sizeof(int32_t));
      for( int j = 0; j < NUM_ELEMENTS; j++){
        arrs[i][j] = random() % INT32_MAX;
      }}

    for( int j = 0; j < NUM_ELEMENTS; j++){
      arrs[0][j] = 0;
    }

    for( int j = 0; j < NUM_ELEMENTS; j++){
      arrs[1][j] = j;
    }


    toggle_m5_region();
    for (int f = 0; f < num_arrs; f++){
      printf("************* %i\n",f);

      for (int i = 0; i < NUM_REPS; i++) {
        memcpy(sort_arr, arrs[f], NUM_ELEMENTS*sizeof(int));
        thrash(garbage, garbage_val++);

        toggle_m5_region();
        asm volatile("CPUID\n\t"
                     "RDTSC\n\t"
                     "mov %%edx, %0\n\t"
                     "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                     "%rax", "%rbx", "%rcx", "%rdx");
        int32_sort(sort_arr, NUM_ELEMENTS);
        asm volatile("RDTSCP\n\t"
                     "mov %%edx, %0\n\t"
                     "mov %%eax, %1\n\n"
                     "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                     "%rax", "%rbx", "%rcx", "%rdx");
        toggle_m5_region();
        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

        timings[i]=end-start;
      }
      FILE* out_f = fopen("djb_sort_basic.out", "a+");
      uint64_t* timing_cur = timings;
      uint64_t* timing_end = timings + (NUM_REPS);
      fprintf(out_f, "\"arr%i\" : [%" PRIu64, f, *timing_cur++ );
      do {
        fprintf(out_f,",%" PRIu64, *timing_cur++);
      }while ( timing_cur != timing_end);
      fprintf(out_f, "], ");
      fclose(out_f);
    }
    for(int f = 0; f < num_arrs; f++){
      FILE* out_f = fopen("djb_sort_arrays.out", "a+");
      int32_t* arr_cur = arrs[f];
      int32_t* arr_end = arr_cur + NUM_ELEMENTS;
      fprintf(out_f, "\"arr_v%i\" : [", f );
      do {
        fprintf(out_f,",%i", *arr_cur++);
      }while ( arr_cur != arr_end);
      fprintf(out_f, "]\n");
      fclose(out_f);
    }

    return 0;
}
