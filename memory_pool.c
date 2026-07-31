#include "memory_pool.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define POOL_SIZE (1024 * 1024)
#define ALIGNMENT 8
#define ALIGN_SIZE(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

typedef struct BlockMeta {
    size_t size;
    bool is_free;
    struct BlockMeta* next;
} BlockMeta;

static uint8_t pool_buffer[POOL_SIZE];
static BlockMeta* block_list_head = NULL;

static void coalesce_free_blocks(void)
{
    BlockMeta* current = block_list_head;

    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(BlockMeta) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void memory_pool_init(void)
{
    block_list_head = (BlockMeta*)pool_buffer;
    block_list_head->size = POOL_SIZE - sizeof(BlockMeta);
    block_list_head->is_free = true;
    block_list_head->next = NULL;
}

void* memory_pool_malloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    size_t aligned_size = ALIGN_SIZE(size);
    BlockMeta* current = block_list_head;

    /* First-fit search */
    while (current != NULL) {
        if (current->is_free && current->size >= aligned_size) {
            if (current->size >= aligned_size + sizeof(BlockMeta) + ALIGNMENT) {
                BlockMeta* new_block = (BlockMeta*)((uint8_t*)current + sizeof(BlockMeta) + aligned_size);
                new_block->size = current->size - aligned_size - sizeof(BlockMeta);
                new_block->is_free = true;
                new_block->next = current->next;

                current->size = aligned_size;
                current->next = new_block;
            }

            current->is_free = false;
            return (void*)(current + 1); /* Return payload pointer */
        }

        current = current->next;
    }

    return NULL; /* Out of memory */
}

void memory_pool_free(void* ptr)
{
    if (ptr == NULL) {
        return;
    }

    BlockMeta* block = ((BlockMeta*)ptr) - 1; /* Get original header */
    block->is_free = true;

    coalesce_free_blocks();
}

void* memory_pool_calloc(size_t num, size_t size)
{
    size_t total_size = num * size;
    void* ptr = memory_pool_malloc(total_size);

    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }

    return ptr;
}

void* memory_pool_realloc(void* ptr, size_t size)
{
    if (ptr == NULL) {
        return memory_pool_malloc(size);
    }

    if (size == 0) {
        memory_pool_free(ptr);
        return NULL;
    }

    BlockMeta* block = ((BlockMeta*)ptr) - 1;

    if (block->size >= size) {
        return ptr; /* Existing block is large enough */
    }

    void* new_ptr = memory_pool_malloc(size);

    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, block->size);
        memory_pool_free(ptr);
    }

    return new_ptr;
}
