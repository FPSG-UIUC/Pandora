/* server program */
#include<vector>
#include<x86intrin.h>
#include<string>
#include<iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
// #include <netinet/in.h>
#include <sys/un.h>

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
void victim_function(volatile char* secret_input, unsigned int len) {
    // secret input is on the stack
    // perform some useless work with it

    // input memory
    volatile char* msg_end = secret_input + len;
    volatile char* msg_cur = secret_input;

#ifdef DEBUG
    for (auto end=secret_input+len, arr=secret_input; arr!=end; arr++) {
        DPRINT(*arr);
    }
    DPRINT('\0');
#endif

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
#define MAX_MSG_LEN 2048

int main(int argc, char*argv[]) {
    // timing
    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;

    int sockfd, newsockfd, servlen;
    socklen_t clilen;

    char buffer[MAX_MSG_LEN];

    // struct sockaddr_un serv_addr, cli_addr;
    struct sockaddr_un  cli_addr, serv_addr;
    int n;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    // portno = atoi(argv[1]);
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, argv[1]);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    // serv_addr.sin_addr.s_addr = INADDR_ANY;
    // serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
        error("ERROR on binding");

    bool stop_server = false;
    while (!stop_server) {
        listen(sockfd,5);
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        bzero(buffer, MAX_MSG_LEN);
        n = read(newsockfd, buffer, MAX_MSG_LEN - 1);
        if (n < 0)
            error("ERROR reading from socket");

        asm volatile("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
                "%rax", "%rbx", "%rcx", "%rdx");
        victim_function(buffer, MAX_MSG_LEN);
        asm volatile("RDTSCP\n\t"
                "mov %%edx, %0\n\t"
                "mov %%eax, %1\n\n"
                "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                "%rax", "%rbx", "%rcx", "%rdx");

        start = ( ((uint64_t)cycles_high << 32) | cycles_low );
        end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

        std::string reply = "I got your message: ";
        reply += std::to_string(end - start);
        n = write(newsockfd, reply.c_str(), reply.size());
        if (n < 0)
            error("ERROR writing to socket");
    }

    close(newsockfd);
    close(sockfd);
    return 0;

}
