#include "memory_pool.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef MEMORY_POOL_SIZE
#define MEMORY_POOL_SIZE (1024U * 1024U)
#endif

#define POOL_SIZE MEMORY_POOL_SIZE
#define ALIGNMENT 8U
#define ALIGN_SIZE(size) (((size) + (ALIGNMENT - 1U)) & ~((size_t)(ALIGNMENT - 1U)))

/**
 * @brief Block metadata header.
 *
 * Doubly-linked structure guarantees O(1) neighbor coalescing without heap traversal.
 * Size is explicitly 16 bytes on 32-bit targets and 32 bytes on 64-bit targets,
 * guaranteeing strict 8-byte payload alignment per MISRA Rule 11.3 and AAPCS.
 */
typedef struct BlockMeta {
    size_t size;
    bool is_free;
    struct BlockMeta* next;
    struct BlockMeta* prev;
} BlockMeta;

/* Static pool storage union to guarantee 8-byte alignment per MISRA Rule 11.3 */
typedef union PoolStorage {
    uint8_t buffer[POOL_SIZE];
    uint64_t alignment_force;
} PoolStorage;

static PoolStorage pool_storage;
#define pool_buffer (pool_storage.buffer)

static BlockMeta* block_list_head = NULL;

void memory_pool_init(void)
{
    block_list_head = (BlockMeta*)(void*)pool_buffer;
    block_list_head->size = POOL_SIZE - sizeof(BlockMeta);
    block_list_head->is_free = true;
    block_list_head->next = NULL;
    block_list_head->prev = NULL;
}

/* Helper to split a block and forward-coalesce the remainder if possible */
static void split_block_if_possible(BlockMeta* block, size_t needed_size)
{
    if (block->size >= (needed_size + sizeof(BlockMeta) + ALIGNMENT)) {
        BlockMeta* new_block = (BlockMeta*)(void*)((uint8_t*)(void*)block + sizeof(BlockMeta) + needed_size);
        new_block->size = block->size - needed_size - sizeof(BlockMeta);
        new_block->is_free = true;
        new_block->next = block->next;
        new_block->prev = block;

        if (new_block->next != NULL) {
            new_block->next->prev = new_block;
        }

        block->size = needed_size;
        block->next = new_block;

        /* Coalesce new_block forward if next neighbor is also free */
        if ((new_block->next != NULL) && (new_block->next->is_free == true)) {
            BlockMeta* next_neighbor = new_block->next;
            new_block->size += sizeof(BlockMeta) + next_neighbor->size;
            new_block->next = next_neighbor->next;

            if (new_block->next != NULL) {
                new_block->next->prev = new_block;
            }
        }
    }
}

void* memory_pool_malloc(size_t size)
{
    void* payload = NULL;

    if (size == 0U) {
        return NULL;
    }

    size_t aligned_size = ALIGN_SIZE(size);
    BlockMeta* current = block_list_head;

    /* First-fit search across blocks */
    while (current != NULL) {
        if ((current->is_free == true) && (current->size >= aligned_size)) {
            split_block_if_possible(current, aligned_size);
            current->is_free = false;
            payload = (void*)(current + 1); /* Return payload pointer */
            break;
        }

        current = current->next;
    }

    return payload;
}

void memory_pool_free(void* ptr)
{
    if (ptr == NULL) {
        return;
    }

    BlockMeta* block = ((BlockMeta*)ptr) - 1;
    block->is_free = true;

    /* O(1) Constant-Time Immediate Forward Coalescing */
    if ((block->next != NULL) && (block->next->is_free == true)) {
        BlockMeta* next_block = block->next;
        block->size += sizeof(BlockMeta) + next_block->size;
        block->next = next_block->next;

        if (block->next != NULL) {
            block->next->prev = block;
        }
    }

    /* O(1) Constant-Time Immediate Backward Coalescing */
    if ((block->prev != NULL) && (block->prev->is_free == true)) {
        BlockMeta* prev_block = block->prev;
        prev_block->size += sizeof(BlockMeta) + block->size;
        prev_block->next = block->next;

        if (block->next != NULL) {
            block->next->prev = prev_block;
        }
    }
}

void* memory_pool_calloc(size_t num, size_t size)
{
    if ((num == 0U) || (size == 0U)) {
        return NULL;
    }

    /* Integer overflow protection per MISRA Rule 12.4 */
    if (num > (SIZE_MAX / size)) {
        return NULL;
    }

    size_t total_size = num * size;
    void* ptr = memory_pool_malloc(total_size);

    if (ptr != NULL) {
        (void)memset(ptr, 0, total_size);
    }

    return ptr;
}

void* memory_pool_realloc(void* ptr, size_t size)
{
    if (ptr == NULL) {
        return memory_pool_malloc(size);
    }

    if (size == 0U) {
        memory_pool_free(ptr);
        return NULL;
    }

    BlockMeta* block = ((BlockMeta*)ptr) - 1;

    size_t aligned_size = ALIGN_SIZE(size);

    if (block->size >= aligned_size) {
        split_block_if_possible(block, aligned_size);
        return ptr;
    }

    /* In-place expansion check: coalesce with adjacent free block if possible */
    if ((block->next != NULL) && (block->next->is_free == true)) {
        size_t combined_size = block->size + sizeof(BlockMeta) + block->next->size;
        if (combined_size >= size) {
            BlockMeta* next_block = block->next;
            block->size = combined_size;
            block->next = next_block->next;

            if (block->next != NULL) {
                block->next->prev = block;
            }

            return ptr;
        }
    }

    void* new_ptr = memory_pool_malloc(size);

    if (new_ptr != NULL) {
        (void)memcpy(new_ptr, ptr, block->size);
        memory_pool_free(ptr);
    }

    return new_ptr;
}
