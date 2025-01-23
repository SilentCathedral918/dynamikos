# Dynamikos

**Dynamikos** is a light-weight single-threaded memory allocator tailored for performance-critical scenarios; it focuses on minimal overhead and optimized memory usage patterns.

---

## Features

- **Fast Allocation and Deallocation**: Dynamikos is optimized for minimal allocation overhead and fast memory operations. Both allocation and deallocation are performed in O(1) time.
- **Adaptive Memory Pool**: Dynamikos dynamically adjusts its internal memory pool to suit varying workloads. Memory allocation and deallocation are O(1), while resizing and rebalancing the pool may take O(n) time.
- **Small Footprint**: The allocator is lightweight and efficient, with minimal overhead for both allocation and deallocation, O(1).
- **Dynamic Size Classes**: Predefined size classes optimize memory usage and reduce fragmentation. Lookups are performed in O(log k) time, while allocation and deallocation remain O(1).


---

## Usage
Here’s how to use **Dynamikos** in your code:

#### Constructing the Allocator
Initialize a new allocator with a specified memory pool size (e.g., 1GB):
```c
dk_allocator *allocator = dk_construct(1024 * 1024 * 1024);  // memory pool of 1GB
```

#### Allocating Memory
Allocate memory for an object:
```c
int *ptr = (int *)dk_allocate(allocator, sizeof(int));
```

#### Deallocating Memory
Free memory when it is no longer needed:
```c
dk_deallocate(allocator, ptr, sizeof(int));
```

#### Resetting the Allocator
Clear the allocator without deallocating the pool:
```c
dk_clear(allocator);
```

#### Cleaning Up
Release the allocator and all its resources:
```c
dk_destruct(allocator);
```

---
## Performance Benchmarks
The benchmarks emphasizes small to medium workloads. The allocator was tested with memory pool of 1GB and optimization flag of -O2.

##### Randomly Sized Structs
Simulates allocating, initializing, and freeing small to moderately sized structs with varying sizes to test allocator's handling of diverse workloads.
```c
typedef struct {
    int id;
    float x, y, z;
    char name[32];
} entity;

for (size_t i = 0; i < ITERATIONS; ++i) {
    size_t size = sizeof(entity) + (rand() % 64);
    void *ptr = dk_allocate(allocator, size);
    memset(ptr, 0, size);
    dk_deallocate(allocator, ptr, size);
}
```
Result: **1,000,000** operations completed in **0.077 seconds**

##### Interleaved Allocations
Performs allocations and deallocations in a random order, mimicking real-world workloads where objects are created and destroyed unpredictably.
```c
for (size_t i = 0; i < ITERATIONS; ++i) {
    if (active_count > 0 && rand() % 2 == 0) {
        size_t index = rand() % active_count;
        dk_deallocate(allocator, allocated[index], sizeof(int));
        allocated[index] = allocated[--active_count];
    } else {
        allocated[active_count++] = dk_allocate(allocator, sizeof(int));
    }
}
```
Result: **1,000,000** operations completed in **0.059 seconds**

##### Fragmentation Stress Test
Alternates between allocating small (4-byte) and large (1024-byte) blocks to evaluate the allocator's ability to manage memory fragmentation.
```c
for (size_t i = 0; i < ITERATIONS; ++i) {
    size_t size = (rand() % 2 == 0) ? sizeof(int) : 1024;
    void *ptr = dk_allocate(allocator, size);
    memset(ptr, 0, size);
    dk_deallocate(allocator, ptr, size);
}
```
Result: **1,000,000** operations completed in **0.079 seconds**

##### String Manipulation
Allocates memory for small strings, concatenates them, and frees memory, testing the allocator's ability to handle frequent string operations.
```c
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
```
Result: **1,000,000** operations completed in **0.6 seconds**

##### Mixed Large and Small Allocations
Alternates between allocating small (32 bytes) and large (1KB–1MB) memory blocks, simulating diverse allocation sizes in general workloads.
```c
for (size_t i = 0; i < ITERATIONS; ++i) {
    size_t size = (rand() % 2 == 0) ? 32 : (rand() % 1024) + 1024;  // Small (32 bytes) or large (1024+ bytes) allocation
    void *ptr = dk_allocate(allocator, size);
    memset(ptr, 0, size);                 // Simulate usage
    dk_deallocate(allocator, ptr, size);  // Deallocate
}
```
Result: **1,000,000** operations completed in **0.088 seconds**

##### Object Graphs / Trees
Creates and destroys binary tree structures with random depths to assess the allocator's handling of hierarchical data structures like object graphs.
```c
typedef struct node {
    struct node *left, *right;
    int data;
} node;

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
```
Result: **1,000,000** operations completed in **0.352 seconds**

##### Large Array Allocations
Allocates and deallocates large arrays (1MB–5MB) repeatedly, testing the allocator's performance under heavy memory-intensive operations. Due to the large size of each allocation, this test is particularly demanding and reflects a worst-case scenario for the allocator's performance.
```c
for (size_t i = 0; i < ITERATIONS; ++i) {
    size_t size = 1024 * 1024 * (rand() % 5 + 1);  // Random large size (1MB to 5MB)
    void *ptr = dk_allocate(allocator, size);
    memset(ptr, 0, size);                 // Simulate work
    dk_deallocate(allocator, ptr, size);  // Deallocate
}
```
Result: **1,000,000** operations completed in **84.035 seconds**

**NOTE**: This result highlights the overhead involved in managing large allocations and provides insight into areas that could benefit from further optimization.

##### Large-Scale String Handling
Works with large strings (64–512 bytes each), performing concatenation and manipulation to evaluate handling of text-heavy workloads.
```c
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
```
Result: **1,000,000** operations completed in **0.688 seconds**

---
## Key Takeaways:
- **Dynamikos** shines in scenarios requiring **frequent small allocations** and **mixed operations**.
- Its **lightweight** design ensures **low** initialization & runtime **overhead**.
- For workloads with **massive, complex allocations** (e.g., large arrays), it may trail industrial-grade allocators.




