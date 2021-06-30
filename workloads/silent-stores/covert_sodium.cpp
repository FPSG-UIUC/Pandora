/* victim program */
#include<x86intrin.h>
#include<string.h>
#include<iostream>
#include<fstream>
#include"sodium.h"

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

// make the messages so long that they don't fit in registers
#define MAX_MSG_LEN 2048


int main() {
    DPRINT('\0');
    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;

#define NUM_ROUNDS 256
    uint64_t timing1[NUM_ROUNDS];
    uint64_t timing2[NUM_ROUNDS];
    uint64_t test_timing1[NUM_ROUNDS];
    uint64_t test_timing2[NUM_ROUNDS];

    std::string output;
    std::ofstream out_file;

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
#define MESSAGE_SIZE 256
    unsigned char message1[MESSAGE_SIZE];
    unsigned char message2[MESSAGE_SIZE];
    unsigned char message1_copy[MESSAGE_SIZE];
    randombytes_buf(message1, MESSAGE_SIZE);
    randombytes_buf(message2, MESSAGE_SIZE);
    // Create an independent copy of the m1 plaintext that adversary can "guess"
    memcpy(message1_copy, message1, MESSAGE_SIZE);

    // Ciphertext buffers
    unsigned char ciphertext1[MESSAGE_SIZE + crypto_secretbox_MACBYTES];
    unsigned char ciphertext2[MESSAGE_SIZE + crypto_secretbox_MACBYTES];
    unsigned char ciphertext3[MESSAGE_SIZE + crypto_secretbox_MACBYTES];
    // memcpy(ciphertext1_copy, ciphertext1, MESSAGE_SIZE +
    //         crypto_secretbox_MACBYTES);

    // Nonce buffer
    unsigned char nonce1[crypto_secretbox_NONCEBYTES];
    unsigned char nonce2[crypto_secretbox_NONCEBYTES];


    /* Define our ops here */
    // Any pre-timing prep, eg nonce derivations
#define PRE_PREP {randombytes_buf(nonce1, sizeof nonce1);randombytes_buf(nonce2, sizeof nonce2);}
    // Victim primes state
#define VICTIM_USAGE crypto_secretbox_easy(ciphertext1, message1, MESSAGE_SIZE, nonce1, session1_tx);
    // Adversary guesses wrong (keys, plaintext, etc)
#define ADVERSARY_USAGE_WRONG crypto_secretbox_easy(ciphertext2, message2, MESSAGE_SIZE, nonce2, session2_tx);
    // Adversary guesses right (keys, plaintext, etc)
#define ADVERSARY_USAGE_RIGHT crypto_secretbox_easy(ciphertext3, message1_copy, MESSAGE_SIZE, nonce2, session2_tx);

    const unsigned repetitions = 5;

    out_file.open("sodium_run_wrong.out", std::ofstream::out);
    out_file << "repetition,data\n";
    out_file.close();
    out_file.open("sodium_run_right.out", std::ofstream::out);
    out_file << "repetition,data\n";
    out_file.close();
    out_file.open("test_pre.out", std::ofstream::out);
    out_file << "repetition,data\n";
    out_file.close();
    out_file.open("test.out", std::ofstream::out);
    out_file << "repetition,data\n";
    out_file.close();

    for (unsigned i=0; i<NUM_ROUNDS; i++) {
        PRE_PREP;
        toggle_m5_region();
        asm volatile("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                "%rax", "%rbx", "%rcx", "%rdx");
        // Adversary tries their guess (wrong)
        VICTIM_USAGE;
        asm volatile("RDTSCP\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n"
                "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                "%rax", "%rbx", "%rcx", "%rdx");
        toggle_m5_region();
        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
        test_timing1[i] = end - start;  // victim time
    }

    output = "test_pre," + std::to_string(*test_timing1);
    for (auto end=test_timing1+NUM_ROUNDS, cur=test_timing1 + 1; cur!=end; cur++) {
        output += "," + std::to_string(*cur);
    }
    output += "\n";
    out_file.open("test_pre.out", std::ofstream::app);
    out_file << output;
    out_file.close();

    PRE_PREP;
    for (unsigned i=0; i<NUM_ROUNDS; i++) {
        toggle_m5_region();
        asm volatile("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                "%rax", "%rbx", "%rcx", "%rdx");
        // Adversary tries their guess (wrong)
        VICTIM_USAGE;
        asm volatile("RDTSCP\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n"
                "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                "%rax", "%rbx", "%rcx", "%rdx");
        toggle_m5_region();
        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
        test_timing2[i] = end - start;  // victim time
    }

    output = "test," + std::to_string(*test_timing1);
    for (auto end=test_timing2+NUM_ROUNDS, cur=test_timing2 + 1; cur!=end; cur++) {
        output += "," + std::to_string(*cur);
    }
    output += "\n";
    out_file.open("test.out", std::ofstream::app);
    out_file << output;
    out_file.close();

    std::cout << "Key stuff time\n";
    /****** TRYING GUESSES ******/
    for (unsigned r=0; r<repetitions; r++) {
        for (unsigned i=0; i<NUM_ROUNDS; i++) {
            PRE_PREP;
            // Victim uses their keys
            VICTIM_USAGE;
            toggle_m5_region();
            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            // Adversary tries their guess (wrong)
            ADVERSARY_USAGE_WRONG;
            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                    "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            toggle_m5_region();
            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            timing1[i] = end - start;  // victim time
        }

        for (unsigned i=0; i<NUM_ROUNDS; i++) {
            PRE_PREP;
            VICTIM_USAGE;
            toggle_m5_region();
            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            // Adversary tries their guess (correct, fully)
            ADVERSARY_USAGE_RIGHT;
            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                    "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            toggle_m5_region();
            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            timing2[i] = end - start;  // victim time
        }

        output = "rep_" + std::to_string(r);
        for (auto end=timing1+NUM_ROUNDS, cur=timing1; cur!=end; cur++) {
            output += "," + std::to_string(*cur);
        }
        output += "\n";
        out_file.open("sodium_run_wrong.out", std::ofstream::app);
        out_file << output;
        out_file.close();

        output = "rep_" + std::to_string(r);
        for (auto end=timing2+NUM_ROUNDS, cur=timing2; cur!=end; cur++) {
            output += "," + std::to_string(*cur);
        }
        output += "\n";
        out_file.open("sodium_run_right.out", std::ofstream::app);
        out_file << output;
        out_file.close();
    }
}
