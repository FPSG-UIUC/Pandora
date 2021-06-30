/* victim program */
#include<x86intrin.h>
#include<string.h>
#include<iostream>
#include<fstream>
#include<assert.h>

#ifdef DEBUG
#define dprint(...) fprintf(stderr, __VA_ARGS__)
#define DASSERT(x) assert(x)
#else
#define dprint(...) do{}while(0)
#define DASSERT(x) do{}while(0)
#endif

#ifdef GEM5
#ifdef DEBUG
#error do not use debug flag with gem5 builds
#endif
#define NDASSERT(x) assert(x)
#include "gem5/m5ops.h"
#define toggle_m5_region() m5_work_begin(0, 0); m5_dump_reset_stats(0, 0);
#else
#define NDASSERT(x) do{}while(0)
#define toggle_m5_region() do{}while(0)
#endif

// make the messages so long that they don't fit in registers
#define MAX_MSG_LEN 2048
#define CACHE_SIZE 4096
#define ASSOC 1
#define CL_SIZE 64

void message_zero(volatile char* msg, unsigned int len) {
    char cval = 'A';
    volatile char* end;
    volatile char* arr;
    for (end=msg+len, arr=msg; arr!=end; arr++) {
        *arr = cval++;
        if (cval > 'Z' && cval < 'a') {
            cval = 'a';
        } else if (cval > 'z') {
            cval = 'A';
        }
        // dprint(cval);
    }
}

void message_one(volatile char* msg, unsigned int len) {
    char cval = 'a';
    volatile char* end;
    volatile char* arr;
    for (end=msg+len, arr=msg; arr!=end; arr++) {
        *arr = cval++;
        if (cval > 'Z' && cval < 'a') {
            cval = 'a';
        } else if (cval > 'z') {
            cval = 'A';
        }
        // dprint(cval);
    }
}

void victim_function_cpy(volatile char* source, char* dest, unsigned int len,
        bool victim) {
    // secret input is on the stack
    // perform some useless work with it

    // input memory
    volatile char* msg_end = source + len;
    volatile char* msg_cur = source;

    dprint("victim_function_cpy\n");
#ifdef DEBUG
    for (auto end=msg_end, arr=msg_cur; arr!=end; arr++) {
        dprint("%c", *arr);
    }
    dprint("\n");
#endif

    // stack memory
    // char msg[len];
    char* dest_end = dest + len;
    char* dest_cur = dest;

    // make sure the addresses are the same across calls: they are.
    // dprint("%p", static_cast<void*>(msg));
    // dprint(" -- ");
    // dprint("%p",static_cast<void*>(msg + len));
    // dprint('\n');

    if (len >= 16) {
        dprint("Using 16 unrolled\n");
        while (msg_cur != msg_end && dest_cur != dest_end) {
            dest_cur[ 0] =  msg_cur[ 0];
            dest_cur[ 1] =  msg_cur[ 1];
            dest_cur[ 2] =  msg_cur[ 2];
            dest_cur[ 3] =  msg_cur[ 3];
            dest_cur[ 4] =  msg_cur[ 4];
            dest_cur[ 5] =  msg_cur[ 5];
            dest_cur[ 6] =  msg_cur[ 6];
            dest_cur[ 7] =  msg_cur[ 7];
            dest_cur[ 8] =  msg_cur[ 8];
            dest_cur[ 9] =  msg_cur[ 9];
            dest_cur[10] =  msg_cur[10];
            dest_cur[11] =  msg_cur[11];
            dest_cur[12] =  msg_cur[12];
            dest_cur[13] =  msg_cur[13];
            dest_cur[14] =  msg_cur[14];
            dest_cur[15] =  msg_cur[15];

            dest_cur += 16; msg_cur += 16;
        }
    } else if (len >= 8) {
        dprint("Using 8 unrolled\n");
        dest_cur[ 0] =  msg_cur[ 0];
        dest_cur[16] =  msg_cur[16];
        dest_cur[32] =  msg_cur[32];
        dest_cur[48] =  msg_cur[48];
        dest_cur[64] =  msg_cur[64];
        dest_cur[80] =  msg_cur[80];
        dest_cur[96] =  msg_cur[96];
        dest_cur[112] =  msg_cur[112];

    } else if (len >= 4) {
        dprint("Using 4 unrolled\n");
        dest_cur[ 0] =  msg_cur[ 0];
        dest_cur[16] =  msg_cur[16];
        dest_cur[32] =  msg_cur[32];
        dest_cur[48] =  msg_cur[48];

    } else if (len >= 2) {
        dprint("Using 2 unrolled\n");
        dest_cur[ 0] =  msg_cur[ 0];  // A
        dest_cur[16] =  msg_cur[16];  // S

    } else if (len == 1) {
        dprint("Using 1 unrolled\n");
        dest_cur[ 0] =  msg_cur[ 0];
    }
}

__attribute__((noinline))
void victim_function(volatile char* secret_input, unsigned int len) {
    // secret input is on the stack
    // perform some useless work with it

    // input memory
    volatile char* msg_end = secret_input + len;
    volatile char* msg_cur = secret_input;

    dprint("victim_function_cpy\n");
#ifdef DEBUG
    for (auto end=msg_end, arr=msg_cur; arr!=end; arr++) {
        dprint("%c", *arr);
    }
    dprint("\n");
#endif
    char* buf = (char*)alloca(512);
    // buf[len % 512] = (char)len;

    // stack memory
    char msg[len];
    char* stack_end = msg + len;
    char* stack_cur = msg;

    // make sure the addresses are the same across calls: they are.
    dprint("%p", static_cast<void*>(msg));
    dprint(" -- ");
    dprint("%p", static_cast<void*>(msg + len));
    dprint("\n");

    if (len >= 16) {
        dprint("Using 16 unrolled\n");
        while (msg_cur != msg_end && stack_cur != stack_end) {
            // *stack_cur++ = *msg_cur++;
            stack_cur[ 0] =  msg_cur[ 0];
            stack_cur[ 1] =  msg_cur[ 1];
            stack_cur[ 2] =  msg_cur[ 2];
            stack_cur[ 3] =  msg_cur[ 3];
            stack_cur[ 4] =  msg_cur[ 4];
            stack_cur[ 5] =  msg_cur[ 5];
            stack_cur[ 6] =  msg_cur[ 6];
            stack_cur[ 7] =  msg_cur[ 7];
            stack_cur[ 8] =  msg_cur[ 8];
            stack_cur[ 9] =  msg_cur[ 9];
            stack_cur[10] =  msg_cur[10];
            stack_cur[11] =  msg_cur[11];
            stack_cur[12] =  msg_cur[12];
            stack_cur[13] =  msg_cur[13];
            stack_cur[14] =  msg_cur[14];
            stack_cur[15] =  msg_cur[15];

            stack_cur += 16; msg_cur += 16;
        }
    } else if (len >= 8) {
        dprint("Using 8 unrolled\n");
        stack_cur[ 0] =  msg_cur[ 0];
        stack_cur[ 1] =  msg_cur[ 1];
        stack_cur[ 2] =  msg_cur[ 2];
        stack_cur[ 3] =  msg_cur[ 3];
        stack_cur[ 4] =  msg_cur[ 4];
        stack_cur[ 5] =  msg_cur[ 5];
        stack_cur[ 6] =  msg_cur[ 6];
        stack_cur[ 7] =  msg_cur[ 7];

    } else if (len >= 4) {
        dprint("Using 4 unrolled\n");
        stack_cur[ 0] =  msg_cur[ 0];
        stack_cur[ 1] =  msg_cur[ 1];
        stack_cur[ 2] =  msg_cur[ 2];
        stack_cur[ 3] =  msg_cur[ 3];

    } else if (len >= 2) {
        dprint("Using 2 unrolled\n");
        stack_cur[ 0] =  msg_cur[ 0];
        stack_cur[ 1] =  msg_cur[ 1];

    } else if (len == 1) {
        dprint("Using 1 unrolled\n");
        stack_cur[ 0] =  msg_cur[ 0];
    }
}

void set_long(volatile uint16_t* large_arr) {
    volatile uint16_t* curr = large_arr;
    for (volatile uint16_t* end = large_arr + CACHE_SIZE * 2; curr != end;
            curr++) {
        *curr = 0;
    }
}

void thrash_cache(volatile uint16_t* large_arr) {
    volatile uint16_t* curr = large_arr;
    for (volatile uint16_t* end = large_arr + CACHE_SIZE * 2;
            curr != end; curr += 8) {
        *curr += 20;
    }
}

int main() {
    /* First, the victim puts something on the stack */
    volatile char* sender_zero = (char*)aligned_alloc(64,
            MAX_MSG_LEN * sizeof(char));
    volatile char* sender_one = (char*)aligned_alloc(64,
            MAX_MSG_LEN * sizeof(char));
    volatile char receiver_msg[MAX_MSG_LEN];

    volatile char* victim_msg[2] = {sender_zero, sender_one};
    unsigned selector = 0;

    volatile uint16_t* long_msg = (volatile uint16_t*)malloc(
            sizeof(uint16_t) * CACHE_SIZE * 2);

    volatile char* local_buf = (char*)aligned_alloc(64,
            MAX_MSG_LEN * sizeof(char));
    message_zero(local_buf, MAX_MSG_LEN);

    /* these are the predetermined messages the adversary sends */
    message_zero(sender_zero, MAX_MSG_LEN);
    message_one(sender_one, MAX_MSG_LEN);
    message_one(receiver_msg, MAX_MSG_LEN);

    // int array_sizes[12] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    int array_sizes[3] = {2, 4, 8};

    dprint("Filled array victim\n");
#ifdef DEBUG
    dprint("Bounds are: ");
    for (auto as_end=array_sizes+3, curr=array_sizes; curr!=as_end; curr++) {
        dprint("%i, ", *curr);
    }
    dprint("\n");
#endif

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;

    uint64_t timing[1024];
    uint64_t* curr_time = timing;

    std::string output;

    std::ofstream out_file;
    out_file.open("heap_copy.out", std::ofstream::out);
    out_file << "size,timing\n";
    out_file.close();

    set_long(long_msg);

    volatile uint64_t* data = (volatile uint64_t*)aligned_alloc(64,
            sizeof(uint64_t) * CACHE_SIZE);
    volatile uint64_t* A;
    volatile uint64_t A_prime;

    A = &data[64];

#define GET_SET(x) ((int64_t)x % (CACHE_SIZE / ASSOC))

    int64_t targ_cache_set = GET_SET(local_buf);
    int64_t dat_cache_set = GET_SET(data);
    dprint("%li -- %li\n", targ_cache_set, dat_cache_set);
    *A = targ_cache_set - dat_cache_set;
    if (*A < 0) {
        *A += (CACHE_SIZE / ASSOC);
    }
    *A /= sizeof(uint64_t);

    dprint("%li -- %li (%li)\n", GET_SET(local_buf),
            GET_SET(&data[*A]), *A);
    NDASSERT(GET_SET(local_buf) == GET_SET(&data[*A]));
    dprint("Setup aop/data\n");

    toggle_m5_region();

#define REPETITIONS 16

    for (unsigned i=0; i<REPETITIONS * 2; i++) {
        selector = selector ^ 0x1;

        // victim puts data
        local_buf[ 0] = victim_msg[selector][ 0];
        // local_buf[64] = victim_msg[selector][64];

        _mm_clflush((const void*)A);  // MFENCE

        asm volatile("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                "%rax", "%rbx", "%rcx", "%rdx");

        A_prime = *A;  // A' = load A  // A --> set(A) != set(A')
        // A_prime = data[A_prime];  // load *A --> set(A') == set(S)
        data[A_prime] += 2;  // Store *A --> set(A') == set(S)

        // adversary puts data
        local_buf[ 0] = 'a';
        // local_buf[64] = 'm';

        asm volatile("RDTSCP\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n"
                "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                "%rax", "%rbx", "%rcx", "%rdx");
        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
        *curr_time++ = end - start;  // victim time
    }

    output = std::to_string(*timing);
    for (auto end=timing+REPETITIONS, cur=timing + 1; cur!=end; cur++) {
        output += "," + std::to_string(*cur);
    }
    output += "\n";
    out_file.open("heap_copy.out", std::ofstream::app);
    out_file << output;
    out_file.close();
}
