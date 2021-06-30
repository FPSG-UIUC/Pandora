#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ctaes.h"
/* #include <stdlib.h> */
#include "test_texts.h"

#include <inttypes.h>

#ifdef DEBUG
#include<assert.h>
#define dprint(...) fprintf(stderr, __VA_ARGS__)
// yeah, you have to keep calling it with \0; but it allows you to concatenate
// in prints without littering ifdef debug everywhere *shrug*
/* #define DPRINT(x) if (x == '\0') std::cout << std::endl << "[[DEBUG]]"; else std::cout << (x); */
#define DASSERT(x) assert(x);
#else
#define dprint(...) do{}while(0)
#define DASSERT(x) do {} while(0)
#endif

/* unsigned char test[TEXT_SIZE] = { */
/*     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; */

int main() {
    unsigned char* ptexts = malloc(16);  // num_keys * chars_per_key
    uint32_t counter = 0;
    for (unsigned curr_start=0; curr_start<65536; curr_start+=256) {
        for (unsigned entry=0; entry<256; entry++) {
            dprint("---- %i (%2x) ----\n", counter, counter);
            uint32_t val = counter++;

            for (unsigned byte = 0; byte < 16; byte++) {
                if (val & 0x8000) {
                    dprint("--%x ", ptexts[byte]);
                    ptexts[byte] = right[byte] | 0x80;
                    dprint("%i: %x (%x:1) --> %x (%x)\n", byte, right[byte],
                            val, right[byte] | 0x80, ptexts[byte]);
                } else {
                    dprint("--%x ", ptexts[byte]);
                    ptexts[byte] = right[byte] & 0x7F;
                    dprint("%i: %x (%x:0) --> %x (%x)\n", byte, right[byte],
                            val, right[byte] & 0x7F, ptexts[byte]);
                }
                val = val << 1;
            }

            for (unsigned byte=0; byte<16; byte++) {
                printf("%i,", ptexts[byte]);
            }
            printf("\n");
        }
    }
}
