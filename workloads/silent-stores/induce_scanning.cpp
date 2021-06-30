#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <x86intrin.h>

#ifdef GEM5
#ifdef DEBUG
#error do not use debug flag with gem5 builds
#endif
#include "gem5/m5ops.h"
#define toggle_m5_region()                                                     \
  m5_work_begin(0, 0);                                                         \
  m5_dump_reset_stats(0, 0);
#else
#define toggle_m5_region()                                                     \
  do {                                                                         \
  } while (0)
#endif

const int OVERLAP_AMOUNT = 2;

// Flush the given array from cache
void thrash_cache(volatile int array[], int len) {
  for (unsigned int i = 0; i < len; i += 16) {
    _mm_clflush(const_cast<const int *>(array + i));
  }
}

int slow_to_compute(int x) {
  const int DUMMY_LEN = 128;
  volatile int dummy[DUMMY_LEN];

  // Otherwise the return value will be different

  thrash_cache(dummy, DUMMY_LEN);

  int return_value = x;

  for (int i = 0; i < DUMMY_LEN; i++) {
    return_value |= dummy[i];
  }

  return return_value;
}

// Assumes index i is filled with value i + OVERLAP_AMOUNT - 1
void verify_memory(volatile int array[], int len) {
  printf("[[DEBUG]]\n");
  for (int i = 0; i < len; i++) {
    printf("Index %d : Value %d\n", i, array[i]);
    assert(array[i] == i);
  }
}

int main(int argc, char *argv[]) {
  const int LEN = 256;

  volatile int A[LEN];

  // 1. Testing store-load forwarding
  std::cout << "1. Testing store-load forwarding." << std::endl;

  // Perform interleaved stores that trigger SQ scanning

  volatile int dummy[LEN];

  // initialize memory
  for (int i = 0; i < LEN; i += 16) {
    // must be zero to slow down computation without changing address
    dummy[i] = 0; 
    dummy[i + 1] = i + 1;
    dummy[i + 2] = i + 2;
    dummy[i + 3] = i + 3;
    dummy[i + 4] = i + 4;
    dummy[i + 5] = i + 5;
    dummy[i + 6] = i + 6;
    dummy[i + 7] = i + 7;
    dummy[i + 8] = i + 8;
    dummy[i + 9] = i + 9;
    dummy[i + 10] = i + 10;
    dummy[i + 11] = i + 11;
    dummy[i + 12] = i + 12;
    dummy[i + 13] = i + 13;
    dummy[i + 14] = i + 14;
    dummy[i + 15] = i + 15;
  }

  toggle_m5_region();

  volatile int tmp = 0;
  for (int i = 0; i < LEN; i += 16) {
    tmp = dummy[i]; // slow to resolve

    // TODO interleave a[i|tmp] and a[i]
    // TODO verify no accesses are optimized out

    // slow to compute, assigns NEW value
    // fast to compute, assigns OLD value

    A[i | tmp] = i + 1;
    A[i] = i + 0;

    A[i + 1 | tmp] = i + 2;
    A[i + 1] = i + 1;

    A[i + 2 | tmp] = i + 3;
    A[i + 2] = i + 2;

    A[i + 3 | tmp] = i + 4;
    A[i + 3] = i + 3;

    A[i + 4 | tmp] = i + 5;
    A[i + 4] = i + 4;

    A[i + 5 | tmp] = i + 6;
    A[i + 5] = i + 5;

    A[i + 6 | tmp] = i + 7;
    A[i + 6] = i + 6;

    A[i + 7 | tmp] = i + 8;
    A[i + 7] = i + 7;

    A[i + 8 | tmp] = i + 9;
    A[i + 8] = i + 8;

    A[i + 9 | tmp] = i + 10;
    A[i + 9] = i + 9;

    A[i + 10 | tmp] = i + 11;
    A[i + 10] = i + 10;

    A[i + 11 | tmp] = i + 12;
    A[i + 11] = i + 11;

    A[i + 12 | tmp] = i + 13;
    A[i + 12] = i + 12;

    A[i + 13 | tmp] = i + 14;
    A[i + 13] = i + 13;

    A[i + 14 | tmp] = i + 15;
    A[i + 14] = i + 14;

    A[i + 15 | tmp] = i + 16;
    A[i + 15] = i + 15;

  }

  toggle_m5_region();

  // Memory check

  verify_memory(A, LEN);

  return 0;

  // 2. Test memory-dependent speculation
  std::cout << "2. Testing memory-dependent speculation" << std::endl;

  // Reset arrays
  memset(const_cast<int *>(A), 0, LEN * sizeof(int));
  thrash_cache(A, LEN);

  toggle_m5_region();

  for (int i = 0; i < LEN; i++) {
    uint32_t index = slow_to_compute(i);
    // This store's address will NOT be resolved
    A[index] = i + 1;
    // This store's address will be resolved
    A[i] = i;
  }

  toggle_m5_region();

  // Memory check

  verify_memory(A, LEN);

  return 0;
}
