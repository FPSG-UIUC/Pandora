/* victim program */

#ifdef DEBUG
#include<iostream>
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
#define MSG_LEN 256

void generate_msg(volatile char* msg, unsigned int len, char init = 'A') {
    char cval = init;
    for (auto end=msg+len, arr=msg; arr!=end; arr++) {
        *arr = cval++;
        if (cval > 'Z' && cval < 'a') {
            cval = 'a';
        } else if (cval > 'z') {
            cval = 'A';
        }
        DPRINT(cval);
    }
    DPRINT('\0');
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
        DPRINT(cval);
    }
    DPRINT('\0');
    for (auto end=msg+len, arr=msg; arr!=end; arr+=16) {
        *arr = 0;
    }
}

__attribute__((noinline))
void victim_function(volatile char* secret_input, unsigned int len) {
    // secret input is on the stack
    // perform some useless work with it
    DASSERT(len == MSG_LEN);

    // input memory
    volatile char* msg_end = secret_input + len;
    volatile char* msg_cur = secret_input;

    // stack memory
    char msg[MSG_LEN];
    char* stack_end = msg + MSG_LEN;
    char* stack_cur = msg;

    // make sure the addresses are the same across calls: they are.
    DPRINT(static_cast<void*>(msg));
    DPRINT("--");
    DPRINT(static_cast<void*>(msg + MSG_LEN));
    DPRINT('\0');

    // std::memcpy(msg, secret_input, len);
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
}

int main() {
    DPRINT('\0');
    /* First, the victim puts something on the stack */
    volatile char msg[MSG_LEN];
    volatile char adv_msg[MSG_LEN];

    volatile char adv_msg_wrong[MSG_LEN];

    /* fill the arrays with random characters;
     * After these calls, the contents are the same -- but they're in different
     * places in memory */
    generate_msg(msg, MSG_LEN);
    generate_msg(adv_msg, MSG_LEN);
    generate_diff_msg(adv_msg_wrong, MSG_LEN);

    /* initialize with random accesses */
    victim_function(adv_msg_wrong, MSG_LEN);
    victim_function(adv_msg_wrong, MSG_LEN);
    victim_function(adv_msg_wrong, MSG_LEN);

    for (unsigned i=0; i<64; i++) {
        /* perform call using the secret message; leaving the secret data on
         * the stack */
        toggle_m5_region();  /* work begin ; dump rest */
        victim_function(msg, MSG_LEN);

        /* now, the adversary puts something over the old message in the
         * function.  if the new content is the same, silent stores should make
         * the run time faster than if it's not */

        /* adversary needs to monitor events which happen after/during this
         * call.  Note: only the first call after the victim's matters -- there
         * is no sense in repeating this call multiple times unless the victim
         * call is also repeated. */
        toggle_m5_region();  /* work begin ; dump rest */
        victim_function(adv_msg, MSG_LEN);

        /* compare timing with an incorrect message */
        toggle_m5_region();  /* work begin ; dump rest */
        victim_function(adv_msg_wrong, MSG_LEN);
        toggle_m5_region();  /* work begin ; dump rest */
    }
}
