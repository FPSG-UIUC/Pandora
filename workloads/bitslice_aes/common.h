#include<stdlib.h>

#define L1_SIZE (8 * 1024)
#define L1_ASSOC 4
#define LINE_LENGTH 64
#define LINE_LENGTH_LOG2 6

#define SET_COUNT (L1_SIZE / LINE_LENGTH / L1_ASSOC)
#define GET_SET(x) ((((uintptr_t)x) >> LINE_LENGTH_LOG2) % SET_COUNT)
#define OFFSET (L1_SIZE / L1_ASSOC / sizeof(uint16_t))

extern
volatile uint16_t* data;
