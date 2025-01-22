#include "dynamikos.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DK_ALIGNMENT 16
#define DK_DEFAULT_CAPACITY 10

struct dynamikos_sizeclass {
    void **_blocks;
    size_t _size;
    size_t _num_blocks;
    size_t _capacity;
};
struct dynamikos_allocator {
    size_t _used_mem;
    size_t _capacity;
    dk_sizeclass *_size_classes;
    size_t _num_classes;
    void *_pool;
};

static inline size_t _dk_apply_alignment(const size_t size, const size_t alignment) {
    return ((size + (alignment - 1)) / alignment) * alignment;
}
static void _dk_compute_size_classes(dk_sizeclass *size_classes, size_t *num_classes, const size_t capacity) {
    const float ratio_ = 1.618f;

    size_t current_size_ = 4;
    size_t index_ = 0;

    for (; current_size_ <= capacity; ++index_) {
        if (size_classes)
            size_classes[index_]._size = _dk_apply_alignment(current_size_, DK_ALIGNMENT);

        printf("index: %u _ containing size: %u\n", index_, _dk_apply_alignment(current_size_, DK_ALIGNMENT));

        current_size_ = (size_t)((float)current_size_ * ratio_);
    }

    if (size_classes)
        size_classes[index_]._size = capacity;

    printf("index: %u _ containing size: %u\n", index_, _dk_apply_alignment(capacity, DK_ALIGNMENT));
    *num_classes = ++index_;
}
static inline size_t _dk_log2_size(size_t size) {
    size_t log2_ = 0;

#if __x86_64__
    if (size >= (1ull << 32)) {
        size >>= 32;
        log2_ += 32;
    }
#endif

    if (size >= (1 << 16)) {
        size >>= 16;
        log2_ += 16;
    }
    if (size >= (1 << 8)) {
        size >>= 8;
        log2_ += 8;
    }
    if (size >= (1 << 4)) {
        size >>= 4;
        log2_ += 4;
    }
    if (size >= (1 << 2)) {
        size >>= 2;
        log2_ += 2;
    }
    if (size >= (1 << 1)) {
        size >>= 1;
        log2_ += 1;
    }

    return log2_;
}
static size_t _dk_get_size_class_index(dk_allocator *allocator, const size_t size) {
    const float log2_ratio_ = 0.694f;

    size_t aligned_size_ = _dk_apply_alignment(size, DK_ALIGNMENT);
    size_t log2_size_ = _dk_log2_size(aligned_size_);

    size_t index_ = (size_t)((float)log2_size_ / log2_ratio_);
    while (index_ > 0 && aligned_size_ <= allocator->_size_classes[index_]._size) --index_;
    return (index_ >= allocator->_num_classes) ? allocator->_num_classes - 1 : index_ + 1;
}

dk_allocator *dk_construct(const size_t capacity) {
    if (!capacity)
        return NULL;

    size_t capacity_ = _dk_apply_alignment(capacity, DK_ALIGNMENT);
    size_t num_sizeclasses_ = 0;
    _dk_compute_size_classes(NULL, &num_sizeclasses_, capacity_);

    size_t sizeclasses_size_ = sizeof(dk_sizeclass) * num_sizeclasses_;
    size_t total_size_ = sizeof(dk_allocator) + sizeclasses_size_ + capacity_;

    uintptr_t addr_base_ = (uintptr_t)malloc(total_size_);
    if (!addr_base_)
        return NULL;

    dk_allocator *allocator_ = (dk_allocator *)addr_base_;
    allocator_->_used_mem = 0;
    allocator_->_capacity = capacity_;

    uintptr_t addr_sizeclasses_ = addr_base_ + sizeof(dk_allocator);
    dk_sizeclass *size_classes_ = (dk_sizeclass *)addr_sizeclasses_;
    for (size_t i = 0; i < num_sizeclasses_; ++i)
        memset(&size_classes_[i], 0, sizeof(dk_sizeclass));
    _dk_compute_size_classes(size_classes_, &num_sizeclasses_, capacity_);
    allocator_->_size_classes = size_classes_;
    allocator_->_num_classes = num_sizeclasses_;

    uintptr_t addr_pool_ = addr_sizeclasses_ + sizeclasses_size_;
    allocator_->_pool = (void *)addr_pool_;
    memset(allocator_->_pool, 0, capacity_);

    return allocator_;
}
bool dk_destruct(dk_allocator *allocator) {
    if (!allocator)
        return false;

    for (size_t i = 0; i < allocator->_num_classes; ++i) {
        dk_sizeclass *class_ = &allocator->_size_classes[i];
        if (class_->_blocks) {
            free(class_->_blocks);
            class_->_blocks = NULL;
        }
    }

    memset(allocator, 0, sizeof(dk_allocator) + (sizeof(dk_sizeclass) * allocator->_num_classes) + allocator->_capacity);
    free(allocator);
    allocator = NULL;

    return true;
}
void *dk_allocate(dk_allocator *allocator, const size_t size) {
    if (!allocator || !size)
        return NULL;

    size_t index_ = _dk_get_size_class_index(allocator, size);
    dk_sizeclass *size_class_ = &allocator->_size_classes[index_];

    if (allocator->_used_mem + size_class_->_size > allocator->_capacity)
        return NULL;

    if ((size_class_->_capacity > 0) && (size_class_->_num_blocks > 0)) {
        allocator->_used_mem += size_class_->_size;
        return size_class_->_blocks[--size_class_->_num_blocks];
    }

    void *ptr_ = (void *)((uintptr_t)(allocator->_pool) + allocator->_used_mem);
    allocator->_used_mem += size_class_->_size;

    return ptr_;
}
bool dk_deallocate(dk_allocator *allocator, void *ptr, const size_t size) {
    if (!allocator || !ptr || !size)
        return false;

    const float ratio_ = 1.618f;

    size_t index_ = _dk_get_size_class_index(allocator, size);
    dk_sizeclass *size_class_ = &allocator->_size_classes[index_];

    if (size_class_->_num_blocks == size_class_->_capacity) {
        size_t new_capacity_ = !size_class_->_capacity ? DK_DEFAULT_CAPACITY : (size_t)((float)size_class_->_capacity * ratio_);

        void **new_blocks_ = realloc(size_class_->_blocks, sizeof(void *) * new_capacity_);
        if (!new_blocks_)
            return false;

        size_class_->_blocks = new_blocks_;
        size_class_->_capacity = new_capacity_;
    }

    size_class_->_blocks[size_class_->_num_blocks++] = ptr;
    allocator->_used_mem -= size_class_->_size;

    return true;
}
bool dk_clear(dk_allocator *allocator) {
    if (!allocator)
        return false;

    for (size_t i = 0; i < allocator->_num_classes; ++i)
        allocator->_size_classes[i]._num_blocks = 0;

    memset(allocator->_pool, 0, allocator->_capacity);
    allocator->_used_mem = 0;

    return true;
}
inline size_t dk_get_used_memory(dk_allocator *allocator) {
    return !allocator ? 0 : allocator->_used_mem;
}
inline size_t dk_get_capacity(dk_allocator *allocator) {
    return !allocator ? 0 : allocator->_capacity;
}
inline void *dk_get_memory_pool(dk_allocator *allocator) {
    return !allocator ? NULL : allocator->_pool;
}
