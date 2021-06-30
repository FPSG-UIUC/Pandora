/* victim program */
#include<vector>
#include<x86intrin.h>
#include<string>
#include<iostream>

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

void generate_msg(volatile char* msg, unsigned int len, char init = 'A') {
    char cval = init;
    for (auto end=msg+len, arr=msg; arr!=end; arr++) {
        *arr = cval++;
        if (cval > 'Z' && cval < 'a') {
            cval = 'a';
        } else if (cval > 'z') {
            cval = 'A';
        }
        // DPRINT(cval);
    }
    // DPRINT('\0');
    for (auto end=msg+len, arr=msg; arr!=end; arr+=16) {
        *arr = 0;
    }
}

void generate_diff_msg(volatile char* msg, unsigned int len, char init = 'a') {
    char cval = init;
    for (auto end=msg+len, arr=msg; arr!=end; arr++) {
        *arr = cval++;
        if (cval > 'Z' && cval < 'a') {
            cval = 'a';
        } else if (cval > 'z') {
            cval = 'A';
        }
        // DPRINT(cval);
    }
    // DPRINT('\0');
    for (auto end=msg+len, arr=msg; arr!=end; arr+=16) {
        *arr = 0;
    }
}

__attribute__((noinline))
void victim_function(volatile char* secret_input, unsigned int len) {
    // secret input is on the stack
    // perform some useless work with it

    // input memory
    volatile char* msg_end = secret_input + len;
    volatile char* msg_cur = secret_input;

    // stack memory
    char msg[len];
    char* stack_end = msg + len;
    char* stack_cur = msg;

    // make sure the addresses are the same across calls: they are.
    DPRINT(static_cast<void*>(msg));
    DPRINT("--");
    DPRINT(static_cast<void*>(msg + len));
    DPRINT('\0');

    if (len >= 16) {
        DPRINT("Using 16 unrolled"); DPRINT('\0');
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
        DPRINT("Using 8 unrolled"); DPRINT('\0');
        stack_cur[ 0] =  msg_cur[ 0];
        stack_cur[ 1] =  msg_cur[ 1];
        stack_cur[ 2] =  msg_cur[ 2];
        stack_cur[ 3] =  msg_cur[ 3];
        stack_cur[ 4] =  msg_cur[ 4];
        stack_cur[ 5] =  msg_cur[ 5];
        stack_cur[ 6] =  msg_cur[ 6];
        stack_cur[ 7] =  msg_cur[ 7];

    } else if (len >= 4) {
        DPRINT("Using 4 unrolled"); DPRINT('\0');
        stack_cur[ 0] =  msg_cur[ 0];
        stack_cur[ 1] =  msg_cur[ 1];
        stack_cur[ 2] =  msg_cur[ 2];
        stack_cur[ 3] =  msg_cur[ 3];

    } else if (len >= 2) {
        DPRINT("Using 2 unrolled"); DPRINT('\0');
        stack_cur[ 0] =  msg_cur[ 0];
        stack_cur[ 1] =  msg_cur[ 1];

    } else if (len == 1) {
        DPRINT("Using 1 unrolled"); DPRINT('\0');
        stack_cur[ 0] =  msg_cur[ 0];
    }
}

int main() {
    DPRINT('\0');
    /* First, the victim puts something on the stack */
    volatile char msg[MAX_MSG_LEN];
    volatile char adv_msg[MAX_MSG_LEN];

    volatile char adv_msg_wrong[MAX_MSG_LEN];

    /* fill the arrays with random characters;
     * After these calls, the contents are the same -- but they're in different
     * places in memory */
    generate_msg(msg, MAX_MSG_LEN);
    generate_msg(adv_msg, MAX_MSG_LEN);
    generate_diff_msg(adv_msg_wrong, MAX_MSG_LEN);

    std::vector<unsigned int> array_sizes{1};
    while (array_sizes.back() * 2 <= MAX_MSG_LEN) {
        array_sizes.push_back(array_sizes.back() * 2);
    }

    DPRINT("Filled array victim"); DPRINT('\0');
#ifdef DEBUG
    std::cout << "Bounds are: ";
    for (unsigned i=0; i<array_sizes.size(); i++) {
        std::cout << array_sizes[i] << ", ";
    }
    DPRINT('\0')
#endif

    /* initialize with random accesses */
    victim_function(adv_msg_wrong, MAX_MSG_LEN);
    victim_function(adv_msg_wrong, MAX_MSG_LEN);
    victim_function(adv_msg_wrong, MAX_MSG_LEN);

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;

    uint64_t timing[196];

    for (auto curr=array_sizes.begin(); curr!=array_sizes.end(); curr++) {
        DPRINT("Using size of "); DPRINT(*curr); DPRINT('\0');

        for (unsigned i=0; i<64; i++) {
            /* perform call using the secret message; leaving the secret data
             * on the stack */
            toggle_m5_region();  /* work begin ; dump rest */

            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");

            victim_function(msg, *curr);

            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                        "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");

            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            timing[3*i] = end - start;  // victim time

            /* now, the adversary puts something over the old message in the
             * function.  if the new content is the same, silent stores should
             * make the run time faster than if it's not */

            /* adversary needs to monitor events which happen after/during this
             * call.  Note: only the first call after the victim's matters --
             * there is no sense in repeating this call multiple times unless
             * the victim call is also repeated. */
            toggle_m5_region();  /* work begin ; dump rest */

            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");

            victim_function(adv_msg, *curr);

            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                        "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");

            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            timing[3*i + 1] = end - start;  // correct guess time

            /* compare timing with an incorrect message */
            toggle_m5_region();  /* work begin ; dump rest */

            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");

            victim_function(adv_msg_wrong, *curr);

            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                        "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");

            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            timing[3*i + 2] = end - start;  // incorrect guess time

            toggle_m5_region();  /* work begin ; dump rest */

        }

        /* these outputs are shown on the gem5 version to extract the timing */
        std::string output = "";
        output += "Victim_time_" + std::to_string(*curr) + ",";
        for (unsigned i=0; i<64; i++) {
            output += std::to_string(timing[i*3]) + ",";
        }
        output += '\n';

        output += "Correct_time_" + std::to_string(*curr) + ",";
        for (unsigned i=0; i<64; i++) {
            output += std::to_string(timing[i*3 + 1]) + ",";
        }
        output += '\n';

        output += "Wrong_time_" + std::to_string(*curr) + ",";
        for (unsigned i=0; i<64; i++) {
            output += std::to_string(timing[i*3 + 2]) + ",";
        }
        output += '\n';
        std::cout << output;
    }
}
