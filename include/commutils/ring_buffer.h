#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data release callback used by the ring buffer.
 *
 * This callback is called when data still owned by the buffer must be released:
 * overwritten unread data, or unread data left during destruction.
 *
 * @param ptr Data pointer to release. The implementation never calls this
 * callback with NULL.
 */
typedef void (*ring_buffer_free_fn)(void *ptr);

/**
 * @brief Opaque ring buffer handle.
 */
typedef struct ring_buffer ring_buffer_t;

/**
 * @brief Create a ring buffer.
 *
 * The buffer stores user-provided data pointers (`void *`). It is safe for one
 * producer thread and one consumer thread to call ring_buffer_push() and
 * ring_buffer_try_pop() concurrently. Synchronization is implemented with a
 * mutex.
 *
 * @param capacity Maximum number of valid items stored in the buffer. Must be
 * greater than 0.
 * @param release_fn Custom release callback for overwritten or unread data. If
 * NULL, free() is used by default.
 *
 * @return New ring buffer handle on success, or NULL on failure.
 */
ring_buffer_t *ring_buffer_create(size_t capacity, ring_buffer_free_fn release_fn);

/**
 * @brief Destroy a ring buffer.
 *
 * Any unread data still owned by the buffer is released with the configured
 * release callback before the buffer memory is freed.
 *
 * @note The caller must ensure no other thread is calling ring_buffer_push(),
 * ring_buffer_try_pop(), ring_buffer_size(), or ring_buffer_capacity() while
 * this function runs.
 *
 * @param rb Ring buffer handle. NULL is accepted and ignored.
 */
void ring_buffer_destroy(ring_buffer_t *rb);

/**
 * @brief Push one data pointer into the buffer.
 *
 * On success, ownership of @p data moves to the buffer. Ownership later moves
 * to the consumer if ring_buffer_try_pop() returns it, or it is released by the
 * buffer if overwritten or left unread at destruction time.
 *
 * If the buffer is full, this function overwrites the oldest unread item,
 * releases that overwritten item with the configured release callback, and
 * advances the read position to the next valid item.
 *
 * @param rb Ring buffer handle.
 * @param data Data pointer to store. NULL is allowed; NULL items are stored and
 * popped like normal items, but release callbacks are not called for NULL.
 *
 * @return 0 on success, or -1 if @p rb is NULL.
 */
int ring_buffer_push(ring_buffer_t *rb, void *data);

/**
 * @brief Try to pop the oldest valid data pointer.
 *
 * When this function returns 1, ownership of `*out` moves to the caller. The
 * caller is responsible for releasing it when no longer needed.
 *
 * @param rb Ring buffer handle.
 * @param out Output location for the popped data pointer.
 *
 * @return 1 if an item was popped successfully.
 * @return 0 if the buffer is empty.
 * @return -1 if @p rb or @p out is NULL.
 */
int ring_buffer_try_pop(ring_buffer_t *rb, void **out);

/**
 * @brief Get the fixed capacity of the buffer.
 *
 * @param rb Ring buffer handle.
 *
 * @return Buffer capacity, or 0 if @p rb is NULL.
 */
size_t ring_buffer_capacity(const ring_buffer_t *rb);

/**
 * @brief Get the current number of valid items in the buffer.
 *
 * The returned value is a snapshot taken while holding the internal mutex.
 * Another thread may change the size immediately after this function returns.
 *
 * @param rb Ring buffer handle.
 *
 * @return Current item count, or 0 if @p rb is NULL.
 */
size_t ring_buffer_size(const ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif
