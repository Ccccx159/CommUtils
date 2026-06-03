#include <gtest/gtest.h>

#include "commutils/ring_buffer.h"

#include <atomic>
#include <cstdlib>
#include <thread>

namespace {

std::atomic_size_t released_count;

int *MakeInt(int value)
{
    int *p = static_cast<int *>(std::malloc(sizeof(*p)));
    EXPECT_NE(p, nullptr);
    if (p != nullptr) {
        *p = value;
    }
    return p;
}

void CountedFree(void *ptr)
{
    released_count.fetch_add(1, std::memory_order_relaxed);
    std::free(ptr);
}

class RingBufferTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        released_count.store(0, std::memory_order_relaxed);
    }
};

TEST_F(RingBufferTest, OverwritesOldestItemAndPreservesOrder)
{
    ring_buffer_t *rb = ring_buffer_create(3, CountedFree);
    ASSERT_NE(rb, nullptr);

    EXPECT_EQ(ring_buffer_push(rb, MakeInt(1)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(2)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(3)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(4)), 0);
    EXPECT_EQ(released_count.load(std::memory_order_relaxed), 1U);

    void *out = nullptr;
    ASSERT_EQ(ring_buffer_try_pop(rb, &out), 1);
    EXPECT_EQ(*static_cast<int *>(out), 2);
    CountedFree(out);

    ASSERT_EQ(ring_buffer_try_pop(rb, &out), 1);
    EXPECT_EQ(*static_cast<int *>(out), 3);
    CountedFree(out);

    ASSERT_EQ(ring_buffer_try_pop(rb, &out), 1);
    EXPECT_EQ(*static_cast<int *>(out), 4);
    CountedFree(out);

    EXPECT_EQ(ring_buffer_try_pop(rb, &out), 0);
    ring_buffer_destroy(rb);
    EXPECT_EQ(released_count.load(std::memory_order_relaxed), 4U);
}

TEST_F(RingBufferTest, MultipleOverwritesKeepNewestItems)
{
    ring_buffer_t *rb = ring_buffer_create(2, CountedFree);
    ASSERT_NE(rb, nullptr);

    EXPECT_EQ(ring_buffer_push(rb, MakeInt(1)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(2)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(3)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(4)), 0);
    EXPECT_EQ(released_count.load(std::memory_order_relaxed), 2U);
    EXPECT_EQ(ring_buffer_size(rb), 2U);

    void *out = nullptr;
    ASSERT_EQ(ring_buffer_try_pop(rb, &out), 1);
    EXPECT_EQ(*static_cast<int *>(out), 3);
    CountedFree(out);

    ASSERT_EQ(ring_buffer_try_pop(rb, &out), 1);
    EXPECT_EQ(*static_cast<int *>(out), 4);
    CountedFree(out);

    EXPECT_EQ(ring_buffer_try_pop(rb, &out), 0);
    ring_buffer_destroy(rb);
    EXPECT_EQ(released_count.load(std::memory_order_relaxed), 4U);
}

TEST_F(RingBufferTest, DestroyReleasesUnreadItems)
{
    ring_buffer_t *rb = ring_buffer_create(4, CountedFree);
    ASSERT_NE(rb, nullptr);

    EXPECT_EQ(ring_buffer_push(rb, MakeInt(10)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(11)), 0);
    EXPECT_EQ(ring_buffer_push(rb, MakeInt(12)), 0);

    ring_buffer_destroy(rb);
    EXPECT_EQ(released_count.load(std::memory_order_relaxed), 3U);
}

TEST_F(RingBufferTest, ApiEdges)
{
    void *out = reinterpret_cast<void *>(0x1);

    EXPECT_EQ(ring_buffer_create(0, CountedFree), nullptr);
    EXPECT_EQ(ring_buffer_push(nullptr, nullptr), -1);
    EXPECT_EQ(ring_buffer_try_pop(nullptr, &out), -1);
    EXPECT_EQ(ring_buffer_try_pop(reinterpret_cast<ring_buffer_t *>(0x1), nullptr), -1);
    EXPECT_EQ(ring_buffer_capacity(nullptr), 0U);
    EXPECT_EQ(ring_buffer_size(nullptr), 0U);

    ring_buffer_t *rb = ring_buffer_create(1, CountedFree);
    ASSERT_NE(rb, nullptr);
    EXPECT_EQ(ring_buffer_capacity(rb), 1U);
    EXPECT_EQ(ring_buffer_size(rb), 0U);

    EXPECT_EQ(ring_buffer_push(rb, nullptr), 0);
    EXPECT_EQ(ring_buffer_size(rb), 1U);
    EXPECT_EQ(ring_buffer_try_pop(rb, &out), 1);
    EXPECT_EQ(out, nullptr);
    EXPECT_EQ(ring_buffer_try_pop(rb, &out), 0);

    ring_buffer_destroy(rb);
}

TEST_F(RingBufferTest, SingleProducerSingleConsumerStress)
{
    constexpr int kPushCount = 100000;
    ring_buffer_t *rb = ring_buffer_create(256, CountedFree);
    ASSERT_NE(rb, nullptr);

    std::atomic_bool done(false);
    std::atomic_size_t consumed(0);

    std::thread producer([&] {
        for (int i = 0; i < kPushCount; ++i) {
            ASSERT_EQ(ring_buffer_push(rb, MakeInt(i)), 0);
        }
        done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        while (!done.load(std::memory_order_acquire) || ring_buffer_size(rb) != 0) {
            void *out = nullptr;
            int rc = ring_buffer_try_pop(rb, &out);
            if (rc == 1) {
                CountedFree(out);
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                ASSERT_EQ(rc, 0);
            }
        }
    });

    producer.join();
    consumer.join();

    ring_buffer_destroy(rb);
    EXPECT_EQ(released_count.load(std::memory_order_relaxed), static_cast<size_t>(kPushCount));
    EXPECT_LE(consumed.load(std::memory_order_relaxed), static_cast<size_t>(kPushCount));
}

}  // namespace

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
