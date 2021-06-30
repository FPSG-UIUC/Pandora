/* server program */
#include<vector>
#include<x86intrin.h>
#include<string>
#include<iostream>
#include<fstream>

// #include <stdio.h>
// #include <stdlib.h>
#include <string.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>

#include<alloca.h>

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

void error(const char *msg) {
    perror(msg);
    exit(1);
}

__attribute__((noinline))
void victim_function_cpy(volatile char* source, volatile char* dest,
    unsigned int len) {
    // secret input is on the stack
    // perform some useless work with it

    // input memory
    volatile char* msg_end = source + 4 + len;
    volatile char* msg_cur = source + 4;

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
    volatile char* msg_end = secret_input + 4 + len;
    volatile char* msg_cur = secret_input + 4;

#ifdef DEBUG
    for (auto end=msg_end, arr=msg_cur; arr!=end; arr++) {
        DPRINT(*arr);
    }
    DPRINT('\0');
#endif

    // garbage to add some offset to the stack
    // void* __attribute__((used)) dat = alloca(512);
    // void* [[used]] dat = alloca(512);

    volatile char* buf = (char*)alloca(512);
    buf[len % 512] = (char)len;
    // char tmp = 0;
    // for (auto end=buf+512, cur=buf; cur != end; cur+=16) {
    //   tmp += cur[ 0];
    //   tmp += cur[ 1];
    //   tmp += cur[ 2];
    //   tmp += cur[ 3];
    //   tmp += cur[ 4];
    //   tmp += cur[ 5];
    //   tmp += cur[ 6];
    //   tmp += cur[ 7];
    //   tmp += cur[ 8];
    //   tmp += cur[ 9];
    //   tmp += cur[10];
    //   tmp += cur[11];
    //   tmp += cur[12];
    //   tmp += cur[13];
    //   tmp += cur[14];
    //   tmp += cur[15];
    // }

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

// make the messages so long that they don't fit in registers
#define MAX_MSG_LEN 2052

// int read(std::ifstream ifile, char* buffer, unsigned int len) {
//   if (ifile.is_open()) {
//     ifile.read(buffer, len);
//   }
// }

int main(int argc, char*argv[]) {
    // timing
    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;

    std::ifstream inp_file;
    std::ofstream out_file;

    out_file.open("./input_file");
    out_file.write("\0", 1);
    out_file.close();
    out_file.open("./output_file");
    out_file.write("\0", 1);
    out_file.close();

    // int sockfd, newsockfd, portno;
    // socklen_t clilen;

    char buffer[MAX_MSG_LEN];
    char buffer_cpy[MAX_MSG_LEN];

    // struct sockaddr_in serv_addr, cli_addr;
    // int n;

    // if (argc < 2) {
    //     fprintf(stderr,"ERROR, no port provided\n");
    //     exit(1);
    // }

    // sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // if (sockfd < 0)
    //     error("ERROR opening socket");

    // bzero((char *) &serv_addr, sizeof(serv_addr));
    // portno = atoi(argv[1]);
    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_addr.s_addr = INADDR_ANY;
    // serv_addr.sin_port = htons(portno);
    // if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    //     error("ERROR on binding");

    unsigned int len;

    bool stop_server = false;
    bzero(buffer, MAX_MSG_LEN);
    bzero(buffer_cpy, MAX_MSG_LEN);
    std::cout << "Ready!\n";
    while (!stop_server) {
        do {
          inp_file.open("./input_file");
          inp_file.read(buffer, MAX_MSG_LEN);
          inp_file.close();
        } while (buffer[0] == '\0');
        len = (buffer[0] - '0') * 1000;
        len += (buffer[1] - '0') * 100;
        len += (buffer[2] - '0') * 10;
        len += (buffer[3] - '0');

        if (len >= 16 && len % 16 != 0) {
          len = len - (len % 16) + 16;
        }

        out_file.open("./input_file");
        out_file.write("\0", 1);
        out_file.close();

        // std::cout << buffer[0] << "," << buffer[1] << "," << buffer[2] << ","
        //   << buffer[3] << "\n";
        // std::cout << "Read: " << buffer << " (" << len << ")" << "\n";

        asm volatile("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                "%rax", "%rbx", "%rcx", "%rdx");
        // TODO replace with libsodium
        victim_function(buffer, len);
        // victim_function_cpy(buffer, buffer_cpy, len);
        asm volatile("RDTSCP\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n"
                "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                "%rax", "%rbx", "%rcx", "%rdx");

        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

        std::string reply;
        reply += std::to_string(len);
        reply += ",";
        reply += std::to_string(end - start);
        reply += "\n";
        std::cout << reply;
        out_file.open("./output_file");
        out_file.write(reply.c_str(), reply.size());
        out_file.close();
    }

    return 0;

}
