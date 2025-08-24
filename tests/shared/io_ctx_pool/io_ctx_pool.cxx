#include <gtest/gtest.h>

#include <clueapi.hxx>

class io_ctx_pool_tests : public ::testing::Test {
  protected:
    void TearDown() override { pool.stop(); }

    clueapi::shared::io_ctx_pool_t pool;
};

TEST_F(io_ctx_pool_tests, start_and_stop) {
    pool.start(4);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    EXPECT_TRUE(pool.is_running());

    pool.stop();

    EXPECT_FALSE(pool.is_running());

    EXPECT_EQ(pool.def_io_ctx(), nullptr);
}

TEST_F(io_ctx_pool_tests, multiple_start_calls) {
    pool.start(2);

    std::atomic<int> counter{0};

    auto io_ctx = pool.io_ctx();

    ASSERT_NE(io_ctx, nullptr);

    boost::asio::post(io_ctx->get_executor(), [&] { counter++; });

    pool.start(4);

    io_ctx = pool.io_ctx();

    ASSERT_NE(io_ctx, nullptr);

    boost::asio::post(io_ctx->get_executor(), [&] { counter++; });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    pool.stop();

    EXPECT_EQ(counter, 2);
}

TEST_F(io_ctx_pool_tests, multiple_stop_calls) {
    pool.start(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    pool.stop();

    EXPECT_NO_THROW(pool.stop());
}

TEST_F(io_ctx_pool_tests, work_distribution) {
    const int num_threads = 4;

    pool.start(num_threads);

    std::vector<std::thread::id> thread_ids(num_threads);

    std::vector<std::promise<std::thread::id>> promises(num_threads);

    std::vector<std::future<std::thread::id>> futures;

    for (auto& p : promises)
        futures.push_back(p.get_future());

    for (int i = 0; i < num_threads; ++i) {
        auto io_ctx = pool.io_ctx();

        ASSERT_NE(io_ctx, nullptr);

        boost::asio::post(io_ctx->get_executor(), [&, i] { promises[i].set_value(std::this_thread::get_id()); });
    }

    for (int i = 0; i < num_threads; ++i)
        thread_ids[i] = futures[i].get();

    pool.stop();

    std::sort(thread_ids.begin(), thread_ids.end());

    auto last = std::unique(thread_ids.begin(), thread_ids.end());

    EXPECT_EQ(std::distance(thread_ids.begin(), last), num_threads);
}

TEST_F(io_ctx_pool_tests, def_io_ctx_works_independently) {
    pool.start(2);

    std::atomic<bool> def_task_executed{false};
    std::atomic<bool> pool_task_executed{false};

    ASSERT_NE(pool.def_io_ctx(), nullptr);

    boost::asio::post(pool.def_io_ctx()->get_executor(), [&] { def_task_executed = true; });

    auto io_ctx = pool.io_ctx();

    ASSERT_NE(io_ctx, nullptr);

    boost::asio::post(io_ctx->get_executor(), [&] { pool_task_executed = true; });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    pool.stop();

    EXPECT_TRUE(def_task_executed);
    EXPECT_TRUE(pool_task_executed);
}

TEST_F(io_ctx_pool_tests, restart_pool) {
    pool.start(2);

    pool.stop();

    EXPECT_FALSE(pool.is_running());

    EXPECT_EQ(pool.def_io_ctx(), nullptr);

    std::atomic<int> counter{0};

    pool.start(2);

    ASSERT_NE(pool.def_io_ctx(), nullptr);

    boost::asio::post(pool.io_ctx()->get_executor(), [&] { counter.fetch_add(1); });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    pool.stop();

    EXPECT_EQ(counter, 1);
}