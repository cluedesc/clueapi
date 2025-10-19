#include <gtest/gtest.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/redis/logger.hpp>
#include <chrono>
#include <memory>
#include <thread>

#include "clueapi/modules/redis/redis.hxx"

using namespace clueapi::modules::redis;

class redis_connection_tests : public ::testing::Test {
   protected:
    void SetUp() override {
        m_cfg.m_host = "127.0.0.1";
        m_cfg.m_port = "6379";

        m_cfg.m_uuid = "test-uuid";

        m_cfg.m_client_name = "clueapi-tests";

        m_cfg.m_log_level = boost::redis::logger::level::err;

        m_cfg.m_connect_timeout = std::chrono::seconds{3};

        m_cfg.m_health_check_interval = std::chrono::seconds{0};
        m_cfg.m_reconnect_wait_interval = std::chrono::seconds{0};
    }

    std::unique_ptr<connection_t> make_connection(
        boost::asio::io_context& io_ctx, bool override_cfg = false, connection_t::cfg_t cfg = {}) {
        if (!override_cfg)
            cfg = m_cfg;

        return std::make_unique<connection_t>(cfg, io_ctx);
    }

    connection_t::cfg_t m_cfg;
};

TEST_F(redis_connection_tests, async_connect_disconnect) {
    boost::asio::io_context io_ctx;

    auto connection = make_connection(io_ctx);

    EXPECT_NE(connection, nullptr);

    auto fut = boost::asio::co_spawn(
        io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->async_connect();

            EXPECT_TRUE(is_connected);

            auto state = connection->state();

            EXPECT_EQ(connection_t::state_t::connected, state);

            auto is_alive = co_await connection->async_check_alive();

            EXPECT_TRUE(is_alive);

            connection->disconnect();

            state = connection->state();

            EXPECT_EQ(connection_t::state_t::disconnected, state);

            co_return;
        },

        boost::asio::use_future);

    io_ctx.run();

    fut.get();

    io_ctx.stop();
}

TEST_F(redis_connection_tests, connect_async_invalid_host) {
    boost::asio::io_context io_ctx;

    auto cfg = m_cfg;

    cfg.m_host = "192.0.2.1";
    cfg.m_log_level = boost::redis::logger::level::debug;

    auto connection = make_connection(io_ctx, true, cfg);

    EXPECT_NE(connection, nullptr);

    auto fut = boost::asio::co_spawn(
        io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->async_connect();

            EXPECT_FALSE(is_connected);

            auto state = connection->state();

            EXPECT_EQ(connection_t::state_t::error, state);

            auto is_alive = co_await connection->async_check_alive();

            EXPECT_FALSE(is_alive);

            connection->disconnect();

            state = connection->state();

            EXPECT_EQ(connection_t::state_t::disconnected, state);

            co_return;
        },

        boost::asio::use_future);

    io_ctx.run();

    fut.get();

    io_ctx.stop();
}

TEST_F(redis_connection_tests, connect_async_invalid_port) {
    boost::asio::io_context io_ctx;

    auto cfg = m_cfg;

    cfg.m_port = "1234";
    cfg.m_log_level = boost::redis::logger::level::debug;

    auto connection = make_connection(io_ctx, true, cfg);

    EXPECT_NE(connection, nullptr);

    auto fut = boost::asio::co_spawn(
        io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->async_connect();

            EXPECT_FALSE(is_connected);

            auto state = connection->state();

            EXPECT_EQ(connection_t::state_t::error, state);

            auto is_alive = co_await connection->async_check_alive();

            EXPECT_FALSE(is_alive);

            connection->disconnect();

            state = connection->state();

            EXPECT_EQ(connection_t::state_t::disconnected, state);

            co_return;
        },

        boost::asio::use_future);

    io_ctx.run();

    fut.get();

    io_ctx.stop();
}

TEST_F(redis_connection_tests, sync_connect_disconnect) {
    boost::asio::io_context io_ctx;

    auto connection = make_connection(io_ctx);

    EXPECT_NE(connection, nullptr);

    std::jthread io_thread([&io_ctx] {
        auto guard = boost::asio::make_work_guard(io_ctx);

        io_ctx.run();
    });

    auto is_connected = connection->sync_connect();

    EXPECT_TRUE(is_connected);

    auto state = connection->state();

    EXPECT_EQ(connection_t::state_t::connected, state);

    auto is_alive = connection->sync_check_alive();

    EXPECT_TRUE(is_alive);

    connection->disconnect();

    state = connection->state();

    EXPECT_EQ(connection_t::state_t::disconnected, state);

    io_ctx.stop();
}

TEST_F(redis_connection_tests, connect_sync_invalid_host) {
    boost::asio::io_context io_ctx;

    auto cfg = m_cfg;

    cfg.m_host = "192.0.2.1";
    cfg.m_log_level = boost::redis::logger::level::debug;

    auto connection = make_connection(io_ctx, true, cfg);

    EXPECT_NE(connection, nullptr);

    std::jthread io_thread([&io_ctx] {
        auto guard = boost::asio::make_work_guard(io_ctx);

        io_ctx.run();
    });

    auto is_connected = connection->sync_connect();

    EXPECT_FALSE(is_connected);

    auto state = connection->state();

    EXPECT_EQ(connection_t::state_t::error, state);

    auto is_alive = connection->sync_check_alive();

    EXPECT_FALSE(is_alive);

    connection->disconnect();

    state = connection->state();

    EXPECT_EQ(connection_t::state_t::disconnected, state);

    io_ctx.stop();
}

TEST_F(redis_connection_tests, connect_sync_invalid_port) {
    boost::asio::io_context io_ctx;

    auto cfg = m_cfg;

    cfg.m_port = "1234";
    cfg.m_log_level = boost::redis::logger::level::debug;

    auto connection = make_connection(io_ctx, true, cfg);

    EXPECT_NE(connection, nullptr);

    std::jthread io_thread([&io_ctx] {
        auto guard = boost::asio::make_work_guard(io_ctx);

        io_ctx.run();
    });

    auto is_connected = connection->sync_connect();

    EXPECT_FALSE(is_connected);

    auto state = connection->state();

    EXPECT_EQ(connection_t::state_t::error, state);

    auto is_alive = connection->sync_check_alive();

    EXPECT_FALSE(is_alive);

    connection->disconnect();

    state = connection->state();

    EXPECT_EQ(connection_t::state_t::disconnected, state);

    io_ctx.stop();
}