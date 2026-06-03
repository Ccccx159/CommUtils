#include "commutils/ring_buffer.h"

#include <pthread.h>
#include <stdlib.h>

struct ring_buffer {
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    ring_buffer_free_fn release_fn;
    pthread_mutex_t mutex;
    void **slots;
};

static void default_release(void *ptr)
{
    free(ptr);
}

static void release_data(const ring_buffer_t *rb, void *ptr)
{
    if (ptr != NULL) {
        rb->release_fn(ptr);
    }
}

static size_t next_index(const ring_buffer_t *rb, size_t index)
{
    ++index;
    return index == rb->capacity ? 0 : index;
}

ring_buffer_t *ring_buffer_create(size_t capacity, ring_buffer_free_fn release_fn)
{
    ring_buffer_t *rb;

    if (capacity == 0) {
        return NULL;
    }

    rb = (ring_buffer_t *)calloc(1, sizeof(*rb));
    if (rb == NULL) {
        return NULL;
    }

    rb->slots = (void **)calloc(capacity, sizeof(rb->slots[0]));
    if (rb->slots == NULL) {
        free(rb);
        return NULL;
    }

    if (pthread_mutex_init(&rb->mutex, NULL) != 0) {
        free(rb->slots);
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->release_fn = release_fn != NULL ? release_fn : default_release;

    return rb;
}

void ring_buffer_destroy(ring_buffer_t *rb)
{
    void *data;

    if (rb == NULL) {
        return;
    }

    /*
     * 销毁前通过正常 pop 路径取出所有未读数据，再逐个释放。
     * 调用者需要保证此时没有其它线程继续访问 rb。
     */
    while (ring_buffer_try_pop(rb, &data) == 1) {
        release_data(rb, data);
    }

    pthread_mutex_destroy(&rb->mutex);
    free(rb->slots);
    free(rb);
}

int ring_buffer_push(ring_buffer_t *rb, void *data)
{
    void *overwritten = NULL;

    if (rb == NULL) {
        return -1;
    }

    pthread_mutex_lock(&rb->mutex);

    if (rb->count == rb->capacity) {
        /*
         * 缓冲区已满时，head 指向最旧数据，tail 在满环中也指向同一
         * 物理槽位。这里在锁内完成替换和指针推进，消费者不可能同时
         * 读取到被覆盖的旧指针。
         */
        overwritten = rb->slots[rb->tail];
        rb->slots[rb->tail] = data;
        rb->head = next_index(rb, rb->head);
        rb->tail = next_index(rb, rb->tail);
    } else {
        /* 缓冲区未满时，直接写入 tail 并增加有效元素数量。 */
        rb->slots[rb->tail] = data;
        rb->tail = next_index(rb, rb->tail);
        ++rb->count;
    }

    pthread_mutex_unlock(&rb->mutex);

    /*
     * 自定义释放函数可能比较耗时，甚至可能触发用户侧复杂逻辑。
     * 因此旧数据指针在锁内摘出，实际释放放在锁外执行。
     */
    release_data(rb, overwritten);

    return 0;
}

int ring_buffer_try_pop(ring_buffer_t *rb, void **out)
{
    if (rb == NULL || out == NULL) {
        return -1;
    }

    pthread_mutex_lock(&rb->mutex);

    if (rb->count == 0) {
        pthread_mutex_unlock(&rb->mutex);
        return 0;
    }

    /*
     * pop 成功后，数据所有权转移给调用者。
     * 槽位清空后，缓冲区不会再释放这个指针。
     */
    *out = rb->slots[rb->head];
    rb->slots[rb->head] = NULL;
    rb->head = next_index(rb, rb->head);
    --rb->count;

    pthread_mutex_unlock(&rb->mutex);
    return 1;
}

size_t ring_buffer_capacity(const ring_buffer_t *rb)
{
    return rb != NULL ? rb->capacity : 0;
}

size_t ring_buffer_size(const ring_buffer_t *rb)
{
    size_t count;

    if (rb == NULL) {
        return 0;
    }

    /*
     * count 由 push/pop 共同修改，读取时也加锁，避免拿到撕裂或过期的
     * 组合状态。返回后其它线程仍可能立刻改变大小。
     */
    pthread_mutex_lock((pthread_mutex_t *)&rb->mutex);
    count = rb->count;
    pthread_mutex_unlock((pthread_mutex_t *)&rb->mutex);

    return count;
}
