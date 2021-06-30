/*********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <inttypes.h>
#include <x86intrin.h>

#include "common.h"

extern
volatile uint16_t* data;  // defined in ctaes.c

#ifdef DEBUG
#include<assert.h>
#define dprint(...) fprintf(stderr, __VA_ARGS__)
// yeah, you have to keep calling it with \0; but it allows you to concatenate
// in prints without littering ifdef debug everywhere *shrug*
/* #define DPRINT(x) if (x == '\0') std::cout << std::endl << "[[DEBUG]]"; else std::cout << (x); */
#define DASSERT(x) assert(x);
#else
#define dprint(...) do{}while(0)
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

#include "ctaes.h"

#include "test_texts.h"

inline void clear_and_thrash(unsigned char* buf) {
    unsigned char* curr;
    /* clear buffers */
    curr = buf;
    for (unsigned char* end=buf+TEXT_SIZE; curr!=end;
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

    /* volatile uint16_t* dcurr = data; */
    /* for (volatile uint16_t* end=data+2*L1_SIZE; dcurr!=end; dcurr+=4) { */
    /*     dcurr[0] += 8; */
    /*     dcurr[1] += 8; */
    /*     dcurr[2] += 8; */
    /*     dcurr[3] += 8; */
    /* } */

}

inline void thrash(unsigned char* buf, unsigned int dat) {
    unsigned char* curr;
    curr = buf;
    for (unsigned char* end=buf+1024; curr!=end; curr+=16) {
        memset(curr, dat, 16);
    }
}

inline unsigned char get_tainted_byte() {
	char *filename = "input_byte.txt";
    FILE *fptr;

	// Open file
    fptr = fopen(filename, "r");
    if (fptr == NULL) {
        printf("Cannot open file \n");
        exit(1);
    }

	unsigned char ff = fgetc(fptr);
	fclose(fptr);

	return ff;
}

inline void taint_key(unsigned char* clean, unsigned char* tainted, unsigned len) {
	unsigned char ff = get_tainted_byte();

	for (int i = 0; i < len; i++)
		tainted[i] = clean[i] & ff;
}

inline void taint_ptexts(unsigned char** clean, unsigned char** tainted, unsigned num_texts, unsigned text_size) {
	unsigned char ff = get_tainted_byte();

	for (int i = 0; i < num_texts; i++)
		for (int j = 0; j < text_size; j++)
			tainted[i][j] = clean[i][j] & ff;
}

int main(int argc, char** argv) {
    unsigned char v_ciphered[TEXT_SIZE];
    unsigned char a_ciphered[TEXT_SIZE];

    unsigned int garbage_val = 0;
    unsigned char* garbage = malloc(1024);  // for clearing the cache

	unsigned char v_key_tainted[KEY_SIZE];
	unsigned char v_key_off_tainted[KEY_SIZE];
	unsigned char** all_ptexts_tainted = all_ptexts_cpy;

    if (argc < 2) {
        printf("Missing input\n");
        return -1;
    }

    uint32_t reps = atoi(argv[1]);

    data = (volatile uint16_t*)aligned_alloc(LINE_LENGTH, sizeof(uint16_t)
            * 4 * L1_SIZE);
    dprint("data: %p\n", data);

    /* if (reps > MAX_REPS) { */
    /*     printf("Too many reps\n"); */
    /*     return -1; */
    /* } */

    /* if (range % NUM_TESTS != 0) { */
    /*     printf("For best results use multiples of %i for the range\n", */
    /*             NUM_TESTS); */
    /*     return -1; */
    /* } */

	taint_key(v_key, v_key_tainted, KEY_SIZE);
	taint_key(v_key_off, v_key_off_tainted, KEY_SIZE);
	taint_ptexts(all_ptexts, all_ptexts_tainted, 100, TEXT_SIZE);

    unsigned char v_key_cpy[KEY_SIZE];
    memcpy(v_key_cpy, v_key, KEY_SIZE);

    AES128_ctx v_ctx, a_ctx, v_ctx_cpy, v_ctx_off;
    AES128_init(&v_ctx, v_key_tainted);
    AES128_init(&a_ctx, a_key);
    AES128_init(&v_ctx_cpy, v_key_cpy);
    AES128_init(&v_ctx_off, v_key_off);

    AES128_ctx* a_ctx_selector[2];
    a_ctx_selector[1] = &a_ctx;
    a_ctx_selector[0] = &v_ctx_cpy;

	AES128_ctx* v_ctx_selector[2];
	v_ctx_selector[0] = &v_ctx;
	v_ctx_selector[1] = &v_ctx_off;

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;
    uint64_t right_timing[MAX_REPS * NUM_TESTS];
    uint64_t wrong_timing[MAX_REPS * NUM_TESTS];
    /* uint64_t* right_timing = (uint64_t*)malloc(MAX_REPS * NUM_TESTS * */
    /*         sizeof(uint64_t)); */
    /* uint64_t* wrong_timing = (uint64_t*)malloc(MAX_REPS * NUM_TESTS * */
    /*         sizeof(uint64_t)); */

    unsigned right_run = 0, wrong_run = 0;
    unsigned guess_type = 0;

    /* unsigned char* a_ptexts[NUM_TESTS]; */
    /*  */
    /* for (unsigned entry = 0; entry < NUM_TESTS; entry++) { */
    /*     a_ptexts[entry] = &(sweep_keys[(range + entry) * 16]); */
    /* } */
    /* dprint("Finished setup\n"); */

    char out_fname[100];
    strcpy(out_fname, "bslice_timing_");
    strcat(out_fname, argv[1]);
#ifndef GEM5
    strcat(out_fname, ".out");
    FILE* out_f = fopen(out_fname, "w+");
#else
    strcat(out_fname, ".gem5_out");
    FILE* out_f = fopen(out_fname, "w+");
#endif

    // 0
    toggle_m5_region();

    /* volatile unsigned type = 0; */

    for (int j = 0; j < NUM_TESTS; j++) {

        for (int i = 0; i < reps*2; i++) {

            /* flip back and forth on guess type */
            guess_type = guess_type ^ 0x1;

            asm volatile (
                    "movl $0xdead, %%eax\n\t"
                    "movl %[j], %%eax\n\t"
                    :: [j] "r" (j) : "eax"
                    );

            /* victim access */
            // 1
            // 4, 7, 10, 13, 16 | 19,
            toggle_m5_region();
            AES128_encrypt(v_ctx_selector[guess_type], 1, v_ciphered,
                    all_ptexts_tainted[0]);
            toggle_m5_region();

            if (guess_type == 0) {
                asm volatile (
                        "movl $0xfeed, %%eax\n\t"
                        ::: "eax"
                        );
                /* type = 0xfeed; */
            } else {
                asm volatile (
                        "movl $0xbeef, %%eax\n\t"
                        ::: "eax"
                        );
                /* type = 0xbeef; */
            }

            thrash(garbage, garbage_val++);

            /* adversary guess correct */
            // 2, 5, 8, 11, 14 | 17, 20
            toggle_m5_region();
            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            AES128_encrypt(a_ctx_selector[1], 1, a_ciphered, a_ptexts[j]);
            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                    "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            toggle_m5_region();
            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            if(guess_type == 0){
                right_timing[right_run++] = end - start;  // victim time
            }
            else{
                wrong_timing[wrong_run++] = end - start;  // victim time
            }

            asm volatile (
                    "movl $0xbead, %%eax\n\t"
                    ::: "eax"
                    );
            clear_and_thrash(v_ciphered);
            clear_and_thrash(a_ciphered);
            asm volatile (
                    "movl $0xbead, %%eax\n\t"
                    ::: "eax"
                    );

            /* toggle_m5_region(); */

            dprint("main data: %p\n", data);
            assert(data);
        }
    }

    /* return 0; */

    uint64_t* timing_cur = right_timing;
    uint64_t* timing_end = right_timing + (reps * NUM_TESTS);
    fprintf(out_f, "correct,%" PRIu64, *timing_cur++);
    do {
        fprintf(out_f, ",%" PRIu64, *timing_cur++);
    } while (timing_cur != timing_end);
    fprintf(out_f, "\n");

    timing_cur = wrong_timing;
    timing_end = wrong_timing + (reps * NUM_TESTS);
    fprintf(out_f, "wrong,%" PRIu64, *timing_cur++);
    do {
        fprintf(out_f, ",%" PRIu64, *timing_cur++);
    } while (timing_cur != timing_end);
    fprintf(out_f, "\n");

    free((void*)data);

    return 0;
}
