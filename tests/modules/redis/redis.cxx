#include <gtest/gtest.h>

#include <clueapi.hxx>

using namespace clueapi::modules::redis;

class redis_tests : public ::testing::Test {
   protected:
    void SetUp() override {
        m_cfg.m_host = "127.0.0.1";

        m_cfg.m_port = "6379";

        {
            m_conn_cfg.m_host = "127.0.0.1";
            m_conn_cfg.m_port = "6379";

            m_conn_cfg.m_uuid = "test-uuid";

            m_conn_cfg.m_client_name = "clueapi-tests";

            m_conn_cfg.m_log_level = boost::redis::logger::level::err;

            m_conn_cfg.m_connect_timeout = std::chrono::seconds{3};

            m_conn_cfg.m_health_check_interval = std::chrono::seconds{0};
            m_conn_cfg.m_reconnect_wait_interval = std::chrono::seconds{0};
        }

        m_redis.init(m_cfg, &m_io_ctx);
    }

    void TearDown() override {
        m_redis.shutdown();
    }

    c_redis m_redis;

    cfg_t m_cfg;

    connection_t::cfg_t m_conn_cfg;

    boost::asio::io_context m_io_ctx;
};

TEST_F(redis_tests, create_connection) {
    auto connection = m_redis.create_connection(std::nullopt);

    EXPECT_NE(connection, nullptr);

    auto state = connection->state();

    EXPECT_EQ(connection_t::state_t::idle, state);

    auto fut = boost::asio::co_spawn(
        m_io_ctx,

        [&]() -> boost::asio::awaitable<void> {
            auto is_connected = co_await connection->async_connect();

            EXPECT_TRUE(is_connected);

            state = connection->state();

            EXPECT_EQ(connection_t::state_t::connected, state);

            auto is_alive = co_await connection->async_check_alive();

            EXPECT_TRUE(is_alive);

            connection->disconnect();

            state = connection->state();

            EXPECT_EQ(connection_t::state_t::disconnected, state);

            co_return;
        },

        boost::asio::use_future);

    m_io_ctx.run();

    fut.get();

    m_io_ctx.stop();
}