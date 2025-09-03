#include <gtest/gtest.h>

#include <clueapi.hxx>

using namespace clueapi::modules::redis;

class redis_connection_tests : public ::testing::Test {
   protected:
    void SetUp() override {
        m_cfg.m_host = "127.0.0.1";
        m_cfg.m_port = "6379";

        m_cfg.m_uuid = "test-uuid";

        m_cfg.m_client_name = "clueapi-tests";

        m_cfg.m_log_level = boost::redis::logger::level::err;

        m_cfg.m_connect_timeout = std::chrono::seconds{5};

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

TEST_F(redis_connection_tests, connect_disconnect) {
    boost::asio::io_context io_ctx;

    auto connection = make_connection(io_ctx);

    EXPECT_NE(connection, nullptr);

    auto fut = boost::asio::co_spawn(
        io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->connect();

            EXPECT_TRUE(is_connected);

            auto state = connection->state();

            EXPECT_EQ(connection_t::state_t::connected, state);

            auto is_alive = co_await connection->check_alive();

            EXPECT_TRUE(is_alive);

            connection->disconnect();

            auto is_disconnected = co_await connection->check_alive();

            state = connection->state();

            EXPECT_EQ(connection_t::state_t::disconnected, state);

            EXPECT_FALSE(is_disconnected);

            co_return;
        },

        boost::asio::use_future);

    io_ctx.run();

    fut.get();

    io_ctx.stop();
}

TEST_F(redis_connection_tests, connect_invalid_host) {
    boost::asio::io_context io_ctx;

    auto cfg = m_cfg;

    cfg.m_host = "127.0.1.1";

    auto connection = make_connection(io_ctx, true, cfg);

    EXPECT_NE(connection, nullptr);

    auto fut = boost::asio::co_spawn(
        io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->connect();

            EXPECT_FALSE(is_connected);

            auto state = connection->state();

            EXPECT_EQ(connection_t::state_t::error, state);

            auto is_alive = co_await connection->check_alive();

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

TEST_F(redis_connection_tests, connect_invalid_port) {
    boost::asio::io_context io_ctx;

    auto cfg = m_cfg;

    cfg.m_port = "1234";

    auto connection = make_connection(io_ctx, true, cfg);

    EXPECT_NE(connection, nullptr);

    auto fut = boost::asio::co_spawn(
        io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->connect();

            EXPECT_FALSE(is_connected);

            auto state = connection->state();

            EXPECT_EQ(connection_t::state_t::error, state);

            auto is_alive = co_await connection->check_alive();

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

TEST_F(redis_connection_tests, test_default_redis_methods) {
    boost::asio::io_context io_ctx;

    auto connection = make_connection(io_ctx);

    EXPECT_NE(connection, nullptr);

    auto fut = boost::asio::co_spawn(
        io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->connect();

            EXPECT_TRUE(is_connected);

            auto state = connection->state();

            EXPECT_EQ(connection_t::state_t::connected, state);

            auto is_alive = co_await connection->check_alive();

            EXPECT_TRUE(is_alive);

            {
                auto key = "test-key";

                auto value = "test-value";

                if (co_await connection->exists(key)) {
                    auto key_del = co_await connection->del(key);

                    EXPECT_TRUE(key_del);
                }

                {
                    auto key_exists = co_await connection->exists(key);

                    EXPECT_FALSE(key_exists);
                }

                {
                    auto key_del = co_await connection->del(key);

                    EXPECT_FALSE(key_del);
                }

                {
                    auto key_set = co_await connection->set(key, value);

                    EXPECT_TRUE(key_set);
                }

                {
                    auto key_get = co_await connection->get<std::string>(key);

                    EXPECT_EQ(value, key_get.value());
                }

                {
                    auto key_expire = co_await connection->expire(key, std::chrono::seconds{1});

                    EXPECT_TRUE(key_expire);
                }

                {
                    auto steady_timer = boost::asio::steady_timer{io_ctx};

                    steady_timer.expires_after(std::chrono::seconds{2});

                    co_await steady_timer.async_wait(boost::asio::use_awaitable);
                }

                {
                    auto key_get = co_await connection->get<std::string>(key);

                    EXPECT_FALSE(key_get.has_value());
                }

                {
                    auto key_set = co_await connection->set(key, value);

                    EXPECT_TRUE(key_set);
                }

                {
                    auto key_del = co_await connection->del(key);

                    EXPECT_TRUE(key_del);
                }

                {
                    auto key_get = co_await connection->get<std::string>(key);

                    EXPECT_FALSE(key_get.has_value());
                }
            }

            connection->disconnect();

            auto is_disconnected = co_await connection->check_alive();

            state = connection->state();

            EXPECT_EQ(connection_t::state_t::disconnected, state);

            EXPECT_FALSE(is_disconnected);

            co_return;
        },

        boost::asio::use_future);

    io_ctx.run();

    fut.get();

    io_ctx.stop();
}