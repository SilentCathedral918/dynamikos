#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dynamikos/dynamikos.h"

#define ITERATIONS 1000000

void run_combined_workload(dk_allocator *allocator) {
    clock_t start_time, end_time;

    // Randomly Sized Structs
    typedef struct {
        int id;
        float x, y, z;
        char name[32];
    } entity;

    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        size_t size = sizeof(entity) + (rand() % 64);
        void *ptr = dk_allocate(allocator, size);
        memset(ptr, 0, size);
        dk_deallocate(allocator, ptr, size);
    }
    end_time = clock();
    printf("Dynamikos - Randomly Sized Structs: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    // Interleaved Allocations and Deallocations
    void *allocated[1000] = {0};
    size_t active_count = 0;

    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        if (active_count > 0 && rand() % 2 == 0) {
            size_t index = rand() % active_count;
            dk_deallocate(allocator, allocated[index], sizeof(int));
            allocated[index] = allocated[--active_count];
        } else {
            allocated[active_count++] = dk_allocate(allocator, sizeof(int));
        }
    }
    end_time = clock();
    printf("Dynamikos - Interleaved Allocations and Deallocations: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    // Fragmentation Stress Test
    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        size_t size = (rand() % 2 == 0) ? sizeof(int) : 1024;
        void *ptr = dk_allocate(allocator, size);
        memset(ptr, 0, size);
        dk_deallocate(allocator, ptr, size);
    }
    end_time = clock();
    printf("Dynamikos - Fragmentation Stress Test: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    // String Manipulation
    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        size_t len1 = rand() % 32 + 1;
        size_t len2 = rand() % 32 + 1;
        char *str1 = dk_allocate(allocator, len1);
        char *str2 = dk_allocate(allocator, len2);
        snprintf(str1, len1, "Hello %zu", i);
        snprintf(str2, len2, "World %zu", i);

        size_t len3 = len1 + len2 + 1;
        char *concat = dk_allocate(allocator, len3);
        snprintf(concat, len3, "%s %s", str1, str2);

        dk_deallocate(allocator, str1, len1);
        dk_deallocate(allocator, str2, len2);
        dk_deallocate(allocator, concat, len3);
    }
    end_time = clock();
    printf("Dynamikos - String Manipulation: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    // Mixed Large and Small Allocations
    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        size_t size = (rand() % 2 == 0) ? 32 : (rand() % 1024) + 1024;  // Small (32 bytes) or large (1024+ bytes) allocation
        void *ptr = dk_allocate(allocator, size);
        memset(ptr, 0, size);                 // Simulate usage
        dk_deallocate(allocator, ptr, size);  // Deallocate
    }
    end_time = clock();
    printf("Dynamikos - Mixed Large and Small Allocations: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    // Object Graphs / Trees
    typedef struct node {
        struct node *left, *right;
        int data;
    } node;

    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        node *root = NULL;
        size_t depth = rand() % 10 + 1;  // Random depth for tree

        // Build a random tree structure with depth
        for (size_t j = 0; j < depth; ++j) {
            node *new_node = dk_allocate(allocator, sizeof(node));
            new_node->data = rand() % 1000;  // Random data
            new_node->left = new_node->right = NULL;

            if (root == NULL) {
                root = new_node;
            } else {
                // Insert the node randomly
                if (rand() % 2 == 0)
                    root->left = new_node;
                else
                    root->right = new_node;
            }
        }

        // Cleanup
        node *temp = root;
        while (temp) {
            node *next = temp->left ? temp->left : temp->right;
            dk_deallocate(allocator, temp, sizeof(node));
            temp = next;
        }
    }
    end_time = clock();
    printf("Dynamikos - Object Graphs / Trees: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    // Large Array Allocations
    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        size_t size = 1024 * 1024 * (rand() % 5 + 1);  // Random large size (1MB to 5MB)
        void *ptr = dk_allocate(allocator, size);
        memset(ptr, 0, size);                 // Simulate work
        dk_deallocate(allocator, ptr, size);  // Deallocate
    }
    end_time = clock();
    printf("Dynamikos - Large Array Allocations: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    // Large-Scale String Handling
    start_time = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        size_t len1 = rand() % 512 + 64;  // 64 to 512 bytes
        size_t len2 = rand() % 512 + 64;  // 64 to 512 bytes

        char *str1 = dk_allocate(allocator, len1);
        char *str2 = dk_allocate(allocator, len2);
        snprintf(str1, len1, "This is string %zu", i);
        snprintf(str2, len2, "and concatenation %zu", i);

        size_t len3 = len1 + len2 + 1;
        char *concat = dk_allocate(allocator, len3);
        snprintf(concat, len3, "%s %s", str1, str2);

        dk_deallocate(allocator, str1, len1);
        dk_deallocate(allocator, str2, len2);
        dk_deallocate(allocator, concat, len3);
    }
    end_time = clock();
    printf("Dynamikos - Large-Scale String Handling: %.6f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);
}

int main() {
    dk_allocator *allocator = dk_construct(1024 * 1024 * 1024);
    if (!allocator) {
        fprintf(stderr, "Failed to construct allocator\n");
        return 1;
    }

    run_combined_workload(allocator);

    dk_destruct(allocator);
    return 0;
}
