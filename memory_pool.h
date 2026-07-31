#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stddef.h>

/**
 * @brief Initializes the static memory pool.
 *
 * This function must be called once before any other memory allocation
 * function is used. It sets up the initial free list and metadata
 * required to manage the static buffer.
 */
void memory_pool_init(void);

/**
 * @brief Allocates a block of memory from the static pool.
 *
 * Searches for an available block of memory that is large enough to
 * satisfy the request using a first-fit algorithm. If the available
 * block is larger than needed, it may be split.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory block, or NULL if the
 *         allocation fails (e.g., out of memory or size is 0).
 */
void* memory_pool_malloc(size_t size);

/**
 * @brief Deallocates a previously allocated memory block.
 *
 * Returns the block of memory pointed to by ptr back to the pool,
 * making it available for future allocations. It also attempts to
 * coalesce (merge) adjacent free blocks to prevent fragmentation.
 * If ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to the memory block to be freed.
 */
void memory_pool_free(void* ptr);

/**
 * @brief Allocates memory for an array of elements and initializes them to zero.
 *
 * @param num The number of elements to allocate.
 * @param size The size of each element in bytes.
 * @return A pointer to the allocated and zero-initialized memory block,
 *         or NULL if the allocation fails.
 */
void* memory_pool_calloc(size_t num, size_t size);

/**
 * @brief Reallocates a memory block to a new size.
 *
 * Expands or shrinks an existing memory block. If the new size is larger,
 * the new space is uninitialized. If the block cannot be expanded in place,
 * a new block is allocated, the old data is copied, and the old block is freed.
 *
 * @param ptr Pointer to the previously allocated memory block.
 * @param size The new size in bytes.
 * @return A pointer to the reallocated memory block, or NULL if the
 *         request fails.
 */
void* memory_pool_realloc(void* ptr, size_t size);

#endif /* MEMORY_POOL_H */