#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "test_texts.h"

unsigned char test[TEXT_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Invalid input\n");
        return -1;
    }

    uint32_t range = atoi(argv[1]);

    if (range + 256 > 65536) {
        printf("Input too large\n");
        return -1;
    }

    if (range % 256 != 0) {
        printf("For best results use multiples of 256\n");
        return -1;
    }

    for (uint32_t counter = range; counter < range + 256; counter++) {
        printf("---- %i (%2x) ----\n", counter, counter);
        uint32_t val = counter;
        for (unsigned byte = 0; byte < 16; byte++) {
            if (val & 0x8000) {
                printf("%i: %x (%x:1) --> %x\n", byte, test[byte], val,
                        test[byte] | 0x80);
            } else {
                printf("%i: %x (%x:0) --> %x\n", byte, test[byte], val,
                        test[byte] & 0x7F);
            }
            val = val << 1;
        }
    }

    return 0;
}
