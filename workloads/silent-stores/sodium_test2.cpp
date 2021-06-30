/*********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include"sodium.h"

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

#define NUM_TESTS 99
#define  NUM_REPS 20


int main(void) {

    /* Generate a bunch of generally useful keys for libsodium operations */
    // Victim
    unsigned char client1_pk[crypto_kx_PUBLICKEYBYTES];
    unsigned char client1_sk[crypto_kx_SECRETKEYBYTES];
    // Attacker
    unsigned char client2_pk[crypto_kx_PUBLICKEYBYTES];
    unsigned char client2_sk[crypto_kx_SECRETKEYBYTES];
    // Server
    unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];
    unsigned char server_sk[crypto_kx_SECRETKEYBYTES];

    unsigned char session1_rx[crypto_kx_SESSIONKEYBYTES];
    unsigned char session1_tx[crypto_kx_SESSIONKEYBYTES];
    unsigned char session2_rx[crypto_kx_SESSIONKEYBYTES];
    unsigned char session2_tx[crypto_kx_SESSIONKEYBYTES];
    if( sodium_init() < 0){
      printf("Libsodium init failure\n");
      return -1;
    }
    // Generate victim keys
    if( crypto_kx_keypair(client1_pk,client1_sk) != 0){
      printf("Libsodium keypair gen failure\n");
      return -1;
    }
    // Generate attacker keys
    if( crypto_kx_keypair(client2_pk,client2_sk) != 0){
      printf("Libsodium keypair gen failure\n");
      return -1;
    }
    // Generate random server keys
    if( crypto_kx_keypair(server_pk,server_sk) != 0){
      printf("Libsodium keypair gen failure\n");
      return -1;
    }

    // Generate some session keys (serverside)
    crypto_kx_server_session_keys(session1_rx, session1_tx, server_pk, server_sk, client1_pk);
    crypto_kx_server_session_keys(session2_rx, session2_tx, server_pk, server_sk, client1_pk);

    // Plaintexts
    unsigned char message1[MESSAGE_SIZE];
    unsigned char message1_copy[MESSAGE_SIZE];
    randombytes_buf(message1, MESSAGE_SIZE);
    // Create an independent copy of the m1 plaintext that adversary can "guess"
    memcpy(message1_copy, message1, MESSAGE_SIZE);

    // Ciphertext buffers
    unsigned char ciphertext1[MESSAGE_SIZE + crypto_secretbox_MACBYTES];
    unsigned char ciphertext2[MESSAGE_SIZE + crypto_secretbox_MACBYTES];

    // Nonce buffer
    unsigned char nonce1[crypto_secretbox_NONCEBYTES];
    unsigned char nonce2[crypto_secretbox_NONCEBYTES];


    /* Define our ops here */
    // Any pre-timing prep, eg nonce derivations
#define PRE_PREP {randombytes_buf(nonce1, sizeof nonce1);randombytes_buf(nonce2, sizeof nonce2);}
    // Victim primes state
#define VICTIM_USAGE crypto_secretbox_easy(ciphertext1, message1, MESSAGE_SIZE, nonce1, session1_tx);

    unsigned char* sessiontx_select[2] = {session1_tx, session2_tx};

    unsigned int garbage_val = 0;
    unsigned char* garbage = (unsigned char*)malloc(1024);  // for clearing the cache

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;
    uint64_t right_timing[NUM_REPS * NUM_TESTS];
    uint64_t wrong_timing[NUM_REPS * NUM_TESTS];

    unsigned right_run = 0, wrong_run = 0;
    unsigned guess_type = 0;

    // 0
    toggle_m5_region();

    for (int j = 0; j < NUM_TESTS; j++) {

        for (int i = 0; i < NUM_REPS*2; i++) {

          /* flip back and forth on guess type */
          guess_type = guess_type ^ 0x1;
          PRE_PREP;
            /* victim access */
            // 1
            // 4, 7, 10, 13, 16 | 19,
            toggle_m5_region();
            VICTIM_USAGE;
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
            crypto_secretbox_easy(ciphertext2, message1_copy, MESSAGE_SIZE, nonce2, sessiontx_select[guess_type]);
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

            clear_and_thrash(ciphertext1);
            clear_and_thrash(ciphertext2);

        /* toggle_m5_region(); */
        }
    }

#ifndef GEM5
    FILE* out_f = fopen("bslice_timing.out", "w+");
#else
    FILE* out_f = fopen("bslice_timing.gem5_out", "w+");
#endif

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
