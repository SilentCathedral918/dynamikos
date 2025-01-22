#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct dynamikos_sizeclass dk_sizeclass;
typedef struct dynamikos_allocator dk_allocator;

dk_allocator *dk_construct(const size_t capacity);
bool dk_destruct(dk_allocator *allocator);
void *dk_allocate(dk_allocator *allocator, const size_t size);
bool dk_deallocate(dk_allocator *allocator, void *ptr, const size_t size);
bool dk_clear(dk_allocator *allocator);
size_t dk_get_used_memory(dk_allocator *allocator);
size_t dk_get_capacity(dk_allocator *allocator);
void *dk_get_memory_pool(dk_allocator *allocator);
