#include <gtest/gtest.h>

#include <clueapi.hxx>

using namespace clueapi::modules::redis;

class redis_module_tests : public ::testing::Test {
   protected:
    void SetUp() override {
        m_cfg.m_host = "127.0.0.1";
        m_cfg.m_port = "6379";
        m_cfg.m_username = "test_user";
        m_cfg.m_password = "test_pass";
        m_cfg.m_use_ssl = false;
        m_cfg.m_db = 1;
        m_cfg.m_connect_timeout = std::chrono::seconds{5};
        m_cfg.m_health_check_interval = std::chrono::seconds{10};
        m_cfg.m_reconnect_wait_interval = std::chrono::seconds{2};
        m_cfg.m_log_level = boost::redis::logger::level::info;
    }

    void TearDown() override {
        m_redis.shutdown();
    }

    cfg_t m_cfg;

    c_redis m_redis;
};

TEST_F(redis_module_tests, init) {
    boost::asio::io_context io_ctx{};

    EXPECT_FALSE(m_redis.is_running());

    m_redis.init(m_cfg, &io_ctx);

    EXPECT_TRUE(m_redis.is_running());

    const auto& stored_cfg = m_redis.cfg();

    EXPECT_EQ(stored_cfg.m_host, "127.0.0.1");
    EXPECT_EQ(stored_cfg.m_port, "6379");
}

TEST_F(redis_module_tests, shutdown_after_init) {
    boost::asio::io_context io_ctx{};

    m_redis.init(m_cfg, &io_ctx);

    EXPECT_TRUE(m_redis.is_running());

    m_redis.shutdown();

    EXPECT_FALSE(m_redis.is_running());
}

TEST_F(redis_module_tests, shutdown_without_init) {
    EXPECT_FALSE(m_redis.is_running());

    EXPECT_NO_THROW(m_redis.shutdown());

    EXPECT_FALSE(m_redis.is_running());
}

TEST_F(redis_module_tests, multiple_shutdown_calls) {
    boost::asio::io_context io_ctx{};

    m_redis.init(m_cfg, &io_ctx);

    EXPECT_TRUE(m_redis.is_running());

    m_redis.shutdown();

    EXPECT_FALSE(m_redis.is_running());

    EXPECT_NO_THROW(m_redis.shutdown());

    EXPECT_FALSE(m_redis.is_running());
}

TEST_F(redis_module_tests, create_connection_without_init) {
    EXPECT_FALSE(m_redis.is_running());

    auto connection = m_redis.create_connection();

    EXPECT_EQ(connection, nullptr);
}

TEST_F(redis_module_tests, create_connection_after_shutdown) {
    boost::asio::io_context io_ctx{};

    m_redis.init(m_cfg, &io_ctx);

    EXPECT_TRUE(m_redis.is_running());

    m_redis.shutdown();

    EXPECT_FALSE(m_redis.is_running());

    auto connection = m_redis.create_connection();

    EXPECT_EQ(connection, nullptr);
}

TEST_F(redis_module_tests, create_connection_with_default_config) {
    boost::asio::io_context io_ctx{};

    m_redis.init(m_cfg, &io_ctx);

    auto connection = m_redis.create_connection();

    ASSERT_NE(connection, nullptr);

    const auto& conn_cfg = connection->cfg();

    EXPECT_EQ(conn_cfg.m_host, "127.0.0.1");
    EXPECT_EQ(conn_cfg.m_port, "6379");
    EXPECT_EQ(conn_cfg.m_username, "test_user");
    EXPECT_EQ(conn_cfg.m_password, "test_pass");

    EXPECT_FALSE(conn_cfg.m_use_ssl);

    EXPECT_EQ(conn_cfg.m_db, 1);
    EXPECT_EQ(conn_cfg.m_client_name, "clueapi-redis-client");

    EXPECT_FALSE(conn_cfg.m_uuid.empty());
}

TEST_F(redis_module_tests, create_connection_with_custom_config) {
    boost::asio::io_context io_ctx{};

    m_redis.init(m_cfg, &io_ctx);

    connection_t::cfg_t custom_cfg{};

    custom_cfg.m_host = "127.0.0.1";
    custom_cfg.m_port = "6380";
    custom_cfg.m_username = "custom_user";
    custom_cfg.m_password = "custom_pass";
    custom_cfg.m_use_ssl = true;
    custom_cfg.m_db = 5;
    custom_cfg.m_client_name = "custom-client";
    custom_cfg.m_uuid = "custom-uuid";
    custom_cfg.m_connect_timeout = std::chrono::seconds{10};
    custom_cfg.m_health_check_interval = std::chrono::seconds{20};
    custom_cfg.m_reconnect_wait_interval = std::chrono::seconds{5};
    custom_cfg.m_log_level = boost::redis::logger::level::debug;

    auto connection = m_redis.create_connection(custom_cfg);

    ASSERT_NE(connection, nullptr);

    const auto& conn_cfg = connection->cfg();

    EXPECT_EQ(conn_cfg.m_host, "127.0.0.1");
    EXPECT_EQ(conn_cfg.m_port, "6380");
    EXPECT_EQ(conn_cfg.m_username, "custom_user");
    EXPECT_EQ(conn_cfg.m_password, "custom_pass");
    EXPECT_TRUE(conn_cfg.m_use_ssl);
    EXPECT_EQ(conn_cfg.m_db, 5);
    EXPECT_EQ(conn_cfg.m_client_name, "custom-client");
    EXPECT_EQ(conn_cfg.m_uuid, "custom-uuid");
}

TEST_F(redis_module_tests, create_multiple_connections) {
    boost::asio::io_context io_ctx{};

    m_redis.init(m_cfg, &io_ctx);

    auto connection1 = m_redis.create_connection();
    auto connection2 = m_redis.create_connection();

    ASSERT_NE(connection1, nullptr);
    ASSERT_NE(connection2, nullptr);

    EXPECT_NE(connection1->cfg().m_uuid, connection2->cfg().m_uuid);
}

TEST_F(redis_module_tests, create_connection_with_empty_credentials) {
    cfg_t cfg_no_creds;

    cfg_no_creds.m_host = "127.0.0.1";
    cfg_no_creds.m_port = "6379";

    cfg_no_creds.m_username = "";
    cfg_no_creds.m_password = "";

    boost::asio::io_context io_ctx{};

    m_redis.init(cfg_no_creds, &io_ctx);

    auto connection = m_redis.create_connection();

    ASSERT_NE(connection, nullptr);

    const auto& conn_cfg = connection->cfg();

    EXPECT_EQ(conn_cfg.m_username, "default");

    EXPECT_TRUE(conn_cfg.m_password.empty());
}

TEST_F(redis_module_tests, test_connection_with_external_io_ctx) {
    boost::asio::io_context io_ctx{};

    m_redis.init(m_cfg, &io_ctx);

    {
        connection_t::cfg_t custom_cfg{};

        {
            custom_cfg.m_host = "127.0.0.1";
            custom_cfg.m_port = "6379";

            custom_cfg.m_client_name = "custom-client";
            custom_cfg.m_uuid = "custom-uuid";

            custom_cfg.m_connect_timeout = std::chrono::seconds{3};
            custom_cfg.m_health_check_interval = std::chrono::seconds{0};
            custom_cfg.m_reconnect_wait_interval = std::chrono::seconds{0};

            custom_cfg.m_log_level = boost::redis::logger::level::debug;
        }

        auto connection = m_redis.create_connection(std::move(custom_cfg));

        auto fut = boost::asio::co_spawn(
            io_ctx,

            [conn_ptr = connection]() -> boost::asio::awaitable<void> {
                auto is_connected = co_await conn_ptr->connect();

                EXPECT_TRUE(is_connected);

                auto state = conn_ptr->state();

                EXPECT_EQ(state, connection_t::state_t::connected);

                auto is_alive = co_await conn_ptr->check_alive();

                EXPECT_TRUE(is_alive);

                conn_ptr->disconnect();

                auto is_disconnected = co_await conn_ptr->check_alive();

                EXPECT_FALSE(is_disconnected);

                state = conn_ptr->state();

                EXPECT_EQ(state, connection_t::state_t::disconnected);

                co_return;
            },

            boost::asio::use_future);

        io_ctx.run();

        fut.get();
    }

    m_redis.shutdown();
}