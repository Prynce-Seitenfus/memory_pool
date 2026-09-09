# Memory Pool

A lightweight, deterministic, and freestanding dynamic memory allocator in C99, designed for bare-metal embedded systems, real-time operating systems (RTOS), and safety-critical applications.

---

## Key Features

* **Zero System Calls / Freestanding**: Operates completely within a fixed internal static buffer. Never invokes OS-level allocation primitives (`malloc`, `sbrk`, `mmap`).
* **Deterministic Execution**: Predictable latency with first-fit search and constant-time block splitting.
* **Immediate Defragmentation**: Automatically coalesces adjacent free blocks during `memory_pool_free()` to mitigate fragmentation.
* **Strict Alignment**: Enforces 8-byte boundary alignment on all allocated blocks.
* **Standard Library Parity**: Mirrors the standard C memory API with namespaced equivalents (`malloc`, `free`, `calloc`, `realloc`).
* **MISRA C:2012 Compliant**: Adheres to MISRA C:2012 guidelines, including defensive NULL-pointer checks, unsigned literals (`U` suffixes), explicit casts, and multiplication overflow protection in `calloc`.

---

## API Reference

The public API is declared in [`memory_pool.h`](memory_pool.h):

```c
/**
 * @brief Initializes the static memory pool and internal block metadata.
 * Must be called once before any allocation.
 */
void memory_pool_init(void);

/**
 * @brief Allocates an aligned memory block using a first-fit algorithm.
 * @param size Number of bytes to allocate.
 * @return Pointer to payload, or NULL if out of memory or size is 0.
 */
void* memory_pool_malloc(size_t size);

/**
 * @brief Deallocates a previously allocated block and coalesces neighbors.
 * @param ptr Pointer to memory block to free (NULL-safe).
 */
void memory_pool_free(void* ptr);

/**
 * @brief Allocates and zero-initializes an array of elements.
 * Protected against integer multiplication overflow.
 * @param num Number of elements.
 * @param size Size of each element in bytes.
 * @return Pointer to zeroed memory, or NULL on failure.
 */
void* memory_pool_calloc(size_t num, size_t size);

/**
 * @brief Reallocates an existing block to a new size.
 * Follows POSIX/C standard realloc semantics.
 * @param ptr Existing allocated pointer.
 * @param size New requested size in bytes.
 * @return Pointer to resized memory, or NULL on failure.
 */
void* memory_pool_realloc(void* ptr, size_t size);
```

---

## Usage Example

```c
#include "memory_pool.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    /* Initialize the pool once at startup */
    memory_pool_init();

    /* Allocate memory for 100 uint32_t elements */
    uint32_t* sensor_data = (uint32_t*)memory_pool_calloc(100U, sizeof(uint32_t));
    if (sensor_data == NULL) {
        /* Handle allocation failure */
        return -1;
    }

    sensor_data[0] = 42U;

    /* Reallocate block if more space is needed */
    sensor_data = (uint32_t*)memory_pool_realloc(sensor_data, 200U * sizeof(uint32_t));

    /* Free memory back to the static pool */
    memory_pool_free(sensor_data);

    return 0;
}
```

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
