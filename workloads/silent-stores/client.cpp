/* client program
 * has both victim mode and adversary mode */
#include<vector>
#include<x86intrin.h>
#include<string>
#include<iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
// #include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>

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

void error(const char *msg) {
    perror(msg);
    exit(1);
}

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
    // for (auto end=msg+len, arr=msg; arr!=end; arr+=16) {
    //     *arr = 0;
    // }
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
    // for (auto end=msg+len, arr=msg; arr!=end; arr+=16) {
    //     *arr = 0;
    // }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
       fprintf(stderr, "usage %s sock_fname type length\n", argv[0]);
       exit(0);
    }

    int sockfd, servlen, n;
    struct sockaddr_un serv_addr;
    struct hostent *server;

    char buffer[MAX_MSG_LEN];
    char msg[MAX_MSG_LEN];

    std::vector<unsigned int> array_sizes{1};
    while (array_sizes.back() * 2 <= MAX_MSG_LEN) {
        array_sizes.push_back(array_sizes.back() * 2);
    }

    unsigned int msg_type = atoi(argv[2]);
    unsigned int len = atoi(argv[3]);

    if (len >= array_sizes.size()) {
        printf("Bad input size, should be <%li\n", array_sizes.size());
        exit(0);
    }

    std::cout << "Using len " << array_sizes[len] << "\n";

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, argv[1]);

    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
        error("ERROR connecting");

    if (msg_type == 0) {
        generate_msg(msg, array_sizes[len]);
    } else {
        generate_diff_msg(msg, array_sizes[len]);
    }

    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    uint64_t start, end;

    asm volatile("CPUID\n\t"
            "RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\n": "=r" (cycles_high), "=r" (cycles_low)::
            "%rax", "%rbx", "%rcx", "%rdx");
    n = write(sockfd, msg, array_sizes[len]);
    asm volatile("RDTSCP\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\n"
            "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
            "%rax", "%rbx", "%rcx", "%rdx");
    if (n < 0)
         error("ERROR writing to socket");
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
         error("ERROR reading from socket");
    printf("%s\n", buffer);

    start = ( ((uint64_t)cycles_high << 32) | cycles_low );
    end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
    printf("WTime: %s\n", std::to_string(end - start).c_str());

    close(sockfd);
    return 0;

}
