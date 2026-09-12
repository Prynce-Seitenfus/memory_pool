# Memory Pool

A lightweight, deterministic, and freestanding dynamic memory allocator in C99, designed for bare-metal embedded systems, real-time operating systems (RTOS), and safety-critical applications.

---

## Key Features

* **Zero System Calls / Freestanding**: Operates completely within a caller-provided memory buffer. Never invokes OS-level allocation primitives (`malloc`, `sbrk`, `mmap`) per MISRA C:2012 Rule 21.3.
* **Deterministic RAM Footprint**: Eliminates hidden static `.bss` memory in the library. The application explicitly provisions the storage buffer sized to its exact RAM budget.
* **Deterministic Execution**: Predictable latency with first-fit search and constant-time block splitting.
* **Immediate Defragmentation**: Automatically coalesces adjacent free blocks during `memory_pool_free()` to mitigate fragmentation.
* **Strict Alignment**: Enforces 8-byte boundary alignment on all allocated blocks and handles unaligned input buffers safely.
* **Standard Library Parity**: Mirrors the standard C memory API with namespaced equivalents (`malloc`, `free`, `calloc`, `realloc`).
* **MISRA C:2012 Compliant**: Adheres to MISRA C:2012 guidelines, including defensive NULL-pointer checks, unsigned literals (`U` suffixes), explicit casts, and multiplication overflow protection in `calloc`.

---

## API Reference

The public API is declared in [`memory_pool.h`](memory_pool.h):

```c
/**
 * @brief Initializes the memory pool using a caller-provided memory buffer.
 *
 * @param memory Pointer to the caller-allocated memory buffer.
 * @param size   Total size of the memory buffer in bytes.
 * @return true if initialized successfully, false if parameters are invalid.
 */
bool memory_pool_init(void* memory, size_t size);

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
#include <stdbool.h>

/* Application allocates static pool storage according to its RAM budget */
#define APP_POOL_SIZE (16U * 1024U) /* 16 KB */
static uint8_t s_app_pool_storage[APP_POOL_SIZE] __attribute__((aligned(8)));

int main(void)
{
    /* Initialize the pool with the application buffer */
    if (!memory_pool_init(s_app_pool_storage, sizeof(s_app_pool_storage))) {
        return -1;
    }

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
