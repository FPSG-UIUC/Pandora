/* victim program */
#include<x86intrin.h>
#include<string.h>
#include<iostream>
#include<fstream>

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
/* #include "gem5/m5ops.h" */
/* #define toggle_m5_region() m5_work_begin(0, 0); m5_dump_reset_stats(0, 0); */
/* #else */
#define toggle_m5_region() do {} while(0)
#endif

// make the messages so long that they don't fit in registers
#define MAX_MSG_LEN 2048

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
        // DPRINT(cval);
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
        // DPRINT(cval);
    }
}

void victim_function_cpy(volatile char* source, char* dest, unsigned int len) {
    // secret input is on the stack
    // perform some useless work with it

    // input memory
    volatile char* msg_end = source + len;
    volatile char* msg_cur = source;

#ifdef DEBUG
    for (auto end=msg_end, arr=msg_cur; arr!=end; arr++) {
        DPRINT(*arr);
    }
    DPRINT('\0');
#endif

    // stack memory
    // char msg[len];
    volatile char* dest_end = dest + len;
    volatile char* dest_cur = dest;

    // make sure the addresses are the same across calls: they are.
    // DPRINT(static_cast<void*>(msg));
    // DPRINT("--");
    // DPRINT(static_cast<void*>(msg + len));
    // DPRINT('\0');

    if (len >= 16) {
        DPRINT("Using 16 unrolled"); DPRINT('\0');
        while (msg_cur != msg_end && dest_cur != dest_end) {
            // *stack_cur++ = *msg_cur++;
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
        DPRINT("Using 8 unrolled"); DPRINT('\0');
        dest_cur[ 0] =  msg_cur[ 0];
        dest_cur[ 1] =  msg_cur[ 1];
        dest_cur[ 2] =  msg_cur[ 2];
        dest_cur[ 3] =  msg_cur[ 3];
        dest_cur[ 4] =  msg_cur[ 4];
        dest_cur[ 5] =  msg_cur[ 5];
        dest_cur[ 6] =  msg_cur[ 6];
        dest_cur[ 7] =  msg_cur[ 7];

    } else if (len >= 4) {
        DPRINT("Using 4 unrolled"); DPRINT('\0');
        dest_cur[ 0] =  msg_cur[ 0];
        dest_cur[ 1] =  msg_cur[ 1];
        dest_cur[ 2] =  msg_cur[ 2];
        dest_cur[ 3] =  msg_cur[ 3];

    } else if (len >= 2) {
        DPRINT("Using 2 unrolled"); DPRINT('\0');
        dest_cur[ 0] =  msg_cur[ 0];
        dest_cur[ 1] =  msg_cur[ 1];

    } else if (len == 1) {
        DPRINT("Using 1 unrolled"); DPRINT('\0');
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

    char* buf = (char*)alloca(512);
    buf[len % 512] = (char)len;

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
    DPRINT("[[DEBUG]]");
    /* First, the victim puts something on the stack */
    volatile char sender_zero[MAX_MSG_LEN];
    volatile char sender_one[MAX_MSG_LEN];
    volatile char receiver_msg[MAX_MSG_LEN];

    char* local_buf = (char*)malloc(MAX_MSG_LEN);
    message_zero(local_buf, MAX_MSG_LEN);

    /* these are the predetermined messages the adversary sends */
    message_zero(sender_zero, MAX_MSG_LEN);
    message_one(sender_one, MAX_MSG_LEN);
    message_one(receiver_msg, MAX_MSG_LEN);

    const int chars_in_message = 49;
    const int bits_in_message = chars_in_message * 8;
    char message[chars_in_message] = "WARMthis is a secret message, don't tell anyone!";
    bool message_as_bits[bits_in_message];
    for (unsigned bit_idx=0; bit_idx<bits_in_message; bit_idx++) {
        int msg_idx = (int)(bit_idx / 8);
        DPRINT(msg_idx + 1); DPRINT(": ");
        DPRINT(message[msg_idx]);
        DPRINT('\0');
        message_as_bits[bit_idx] = message[msg_idx] & 0x01;
        message[msg_idx] >>= 1;
    }

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    unsigned total_cycles_low, total_cycles_high, total_cycles_low1,
             total_cycles_high1;
    uint64_t start, end, total_start, total_end;

    uint64_t timing[1024];
    float measured_time;
    const int repetitions = 1;

    bool received_message[1024];
    bool* curr_bit = received_message;
    std::string output;
    std::ofstream out_file;

    // 64 -> 139 cycles
    // 32 -> 86 cycles
    // 16 -> 66 cycles
    const unsigned msg_size = 64;
    const unsigned cycles = 139;

    asm volatile("CPUID\n\t"
            "RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\n": "=r" (total_cycles_high), "=r" (total_cycles_low)::
            "%rax", "%rbx", "%rcx", "%rdx");

    /****** VICTIM_FUNCTION = STACK ******/
    // for each bit in message
    unsigned send_count=0;
    for (auto bit=message_as_bits, end_bit=message_as_bits+bits_in_message;
            bit!=end_bit; bit++) {
        // for many different repetitions
        for (unsigned i=0; i<repetitions; i++) {
            // sender puts a ZERO
            if (*bit) {
                victim_function(sender_one, msg_size);
            } else {
                victim_function(sender_zero, msg_size);
            }

            // receiver times it: if timing is HIGH/SLOW, bit was zero
            asm volatile("CPUID\n\t"
                    "RDTSC\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            victim_function(receiver_msg, msg_size);
            asm volatile("RDTSCP\n\t"
                    "mov %%edx, %0\n\t"
                    "mov %%eax, %1\n\n"
                        "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                    "%rax", "%rbx", "%rcx", "%rdx");
            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            timing[send_count++] = end - start;  // victim time
        }

        measured_time = 0;
        for (auto c_time=timing+send_count-repetitions,
                c_end=timing+send_count; c_time!=c_end;
                c_time++) {
            measured_time += *c_time;
        }
        measured_time /= repetitions;

        *curr_bit = measured_time < cycles;
        curr_bit++;
    }
    asm volatile("RDTSCP\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\n"
                "CPUID\n\t": "=r" (total_cycles_high1), "=r" (total_cycles_low1)::
            "%rax", "%rbx", "%rcx", "%rdx");
    total_start = ( ((uint64_t)total_cycles_high << 32) | total_cycles_low );
    total_end = ( ((uint64_t)total_cycles_high1 << 32) | total_cycles_low1 );

    output = "total_time," + std::to_string(total_end - total_start) +
        "\nSent";
    for (auto bit=message_as_bits, end_bit=message_as_bits+bits_in_message;
            bit!=end_bit; bit++) {
        output += *bit ? ",0" : ",1";
    }
    output += "\nReceived";
    for (auto bit=received_message, end_bit=received_message+bits_in_message;
            bit!=end_bit; bit++) {
        output += *bit ? ",0" : ",1";
    }
    output += "\nRaw";
    for (auto bit_time=timing, end_bit=timing+bits_in_message*repetitions;
            bit_time!=end_bit; bit_time++) {
        output += "," + std::to_string(*bit_time);
    }

    // output = std::to_string(*curr) + ",[" + std::to_string(*timing);
    // for (auto end=timing+1024, cur=timing; cur!=end; cur++) {
    //     output += "," + std::to_string(*cur);
    // }
    // output += "]\n";
    out_file.open("covert_full.out", std::ofstream::out);
    out_file << output;
    out_file.close();
}
