#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dynamikos/dynamikos.h"

#define ITERATIONS 1000000
#define MAX_ALLOC_SIZE (1ull << 25)
#define ACTIVE_ALLOCS 10000

typedef struct {
    void *ptr;
    size_t size;
} allocation_record;

void run_stress_test() {
    srand((unsigned int)time(NULL));  // Seed RNG

    // Step 1: Create allocator
    dk_allocator *allocator = dk_construct(1024 * 1024 * 128);  // 128 MB pool
    if (!allocator) {
        printf("Failed to initialize allocator.\n");
        return;
    }

    // Step 2: Setup allocation records
    allocation_record active_allocs[ACTIVE_ALLOCS] = {0};

    // Step 3: Perform allocations and deallocations
    clock_t start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        // Randomly decide whether to allocate or deallocate
        if (rand() % 2 == 0) {
            // Allocate new block
            size_t alloc_size = (rand() % MAX_ALLOC_SIZE) + 1;
            size_t index = rand() % ACTIVE_ALLOCS;

            if (active_allocs[index].ptr) {
                dk_deallocate(allocator, active_allocs[index].ptr, active_allocs[index].size);
            }

            void *ptr = dk_allocate(allocator, alloc_size);
            if (ptr) {
                active_allocs[index].ptr = ptr;
                active_allocs[index].size = alloc_size;
            }
        } else {
            // Deallocate an existing block
            size_t index = rand() % ACTIVE_ALLOCS;
            if (active_allocs[index].ptr) {
                dk_deallocate(allocator, active_allocs[index].ptr, active_allocs[index].size);
                active_allocs[index].ptr = NULL;
                active_allocs[index].size = 0;
            }
        }
    }
    clock_t end_time = clock();

    // Step 4: Clean up remaining allocations
    for (size_t i = 0; i < ACTIVE_ALLOCS; ++i) {
        if (active_allocs[i].ptr) {
            dk_deallocate(allocator, active_allocs[i].ptr, active_allocs[i].size);
        }
    }

    // Step 5: Print results
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Stress test completed: %d operations in %.6f seconds.\n", ITERATIONS, elapsed_time);

    // Step 6: Destroy allocator
    dk_destruct(allocator);
}

int main(int argc, char **argv) {
    run_stress_test();
    // Stress test completed: 1000000 operations in 0.078000 seconds.
    return 0;
}
