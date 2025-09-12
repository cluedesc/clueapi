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

    cfg.m_host = "192.0.2.1";
    cfg.m_log_level = boost::redis::logger::level::debug;

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

            if (!is_alive)
                co_return;

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
                    auto key_exists = co_await connection->exists(key);

                    if (key_exists) {
                        auto key_del = co_await connection->del(key);

                        EXPECT_TRUE(key_del);
                    }
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

TEST_F(redis_connection_tests, test_other_redis_methods) {
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

            if (!is_alive)
                co_return;

            {
                auto key = "test-key";

                if (co_await connection->exists(key)) {
                    auto key_del = co_await connection->del(key);

                    EXPECT_TRUE(key_del);
                }

                {
                    auto len1 = co_await connection->lpush(key, "one");

                    EXPECT_EQ(len1, 1);

                    auto len2 = co_await connection->lpush(key, "two");

                    EXPECT_EQ(len2, 2);

                    auto len3 = co_await connection->lpush(key, "three");

                    EXPECT_EQ(len3, 3);
                }

                {
                    auto list_full = co_await connection->lrange(key, 0, -1);

                    EXPECT_EQ(list_full.size(), 3);

                    if (list_full.size() == 3) {
                        EXPECT_EQ(list_full[0], "three");
                        EXPECT_EQ(list_full[1], "two");
                        EXPECT_EQ(list_full[2], "one");

                        auto trim_ok = co_await connection->ltrim(key, 0, 1);

                        EXPECT_TRUE(trim_ok);

                        auto list_trimmed = co_await connection->lrange(key, 0, -1);

                        EXPECT_EQ(list_trimmed.size(), 2);

                        if (list_trimmed.size() == 2) {
                            EXPECT_EQ(list_trimmed[0], "three");
                            EXPECT_EQ(list_trimmed[1], "two");

                            auto expire_ok =
                                co_await connection->expire(key, std::chrono::seconds{10});

                            EXPECT_TRUE(expire_ok);

                            auto ttl1 = co_await connection->ttl(key);

                            EXPECT_GT(ttl1, 0);
                        }
                    }

                    auto key_del = co_await connection->del(key);

                    EXPECT_TRUE(key_del);
                }

                {
                    auto hash_key = "test-hash-key";

                    if (co_await connection->exists(hash_key)) {
                        auto key_del = co_await connection->del(hash_key);

                        EXPECT_TRUE(key_del);
                    }

                    std::unordered_map<std::string_view, std::string_view> initial_map = {
                        {"field1", "value1"}, {"field2", "value2"}, {"counter", "10"}};

                    auto hset_result = co_await connection->hset(hash_key, initial_map);

                    EXPECT_EQ(hset_result, 3);

                    auto retrieved_map = co_await connection->hgetall(hash_key);

                    EXPECT_EQ(retrieved_map.size(), 3);

                    EXPECT_EQ(retrieved_map["field1"], "value1");
                    EXPECT_EQ(retrieved_map["field2"], "value2");
                    EXPECT_EQ(retrieved_map["counter"], "10");

                    auto hsetfield_update_result =
                        co_await connection->hsetfield(hash_key, "field1", "new-value1");

                    EXPECT_EQ(hsetfield_update_result, 0);

                    auto hsetfield_add_result =
                        co_await connection->hsetfield(hash_key, "field3", "value3");

                    EXPECT_EQ(hsetfield_add_result, 1);

                    auto updated_map = co_await connection->hgetall(hash_key);

                    EXPECT_EQ(updated_map.size(), 4);

                    EXPECT_EQ(updated_map["field1"], "new-value1");
                    EXPECT_EQ(updated_map["field3"], "value3");

                    auto hincrby_result = co_await connection->hincrby(hash_key, "counter", 5);

                    EXPECT_EQ(hincrby_result, 15);

                    auto map_after_incr = co_await connection->hgetall(hash_key);

                    EXPECT_EQ(map_after_incr["counter"], "15");

                    std::vector<std::string_view> fields_to_del = {"field1", "field3"};

                    auto hdel_result = co_await connection->hdel(hash_key, fields_to_del);

                    EXPECT_EQ(hdel_result, 2);

                    auto final_map = co_await connection->hgetall(hash_key);

                    EXPECT_EQ(final_map.size(), 2);

                    EXPECT_FALSE(final_map.count("field1"));
                    EXPECT_FALSE(final_map.count("field3"));
                    EXPECT_TRUE(final_map.count("field2"));

                    auto key_del = co_await connection->del(hash_key);

                    EXPECT_TRUE(key_del);
                }

                {
                    auto hash_key = "test-hash-key-2";

                    if (co_await connection->exists(hash_key)) {
                        auto key_del = co_await connection->del(hash_key);

                        EXPECT_TRUE(key_del);
                    }

                    {
                        auto hset_result =
                            co_await connection->hsetfield(hash_key, "field_hget", "value_hget");

                        EXPECT_EQ(hset_result, 1);
                    }

                    {
                        auto value_hget = co_await connection->hget(hash_key, "field_hget");

                        EXPECT_TRUE(value_hget.has_value());

                        EXPECT_EQ(value_hget.value(), "value_hget");
                    }

                    {
                        auto value_hget_non_existent =
                            co_await connection->hget(hash_key, "field_non_existent");

                        EXPECT_FALSE(value_hget_non_existent.has_value());
                    }

                    {
                        auto hexists_result = co_await connection->hexists(hash_key, "field_hget");

                        EXPECT_TRUE(hexists_result);
                    }

                    {
                        auto hexists_result_non_existent =
                            co_await connection->hexists(hash_key, "field_non_existent");

                        EXPECT_FALSE(hexists_result_non_existent);
                    }

                    {
                        auto key_del = co_await connection->del(hash_key);

                        EXPECT_TRUE(key_del);
                    }
                }

                {
                    auto counter_key = "test-counter";

                    if (co_await connection->exists(counter_key)) {
                        auto key_del = co_await connection->del(counter_key);

                        EXPECT_TRUE(key_del);
                    }

                    {
                        auto incr_result = co_await connection->incr(counter_key);

                        EXPECT_TRUE(incr_result.has_value());
                        EXPECT_EQ(incr_result.value(), 1);
                    }

                    {
                        auto incr_result = co_await connection->incr(counter_key);

                        EXPECT_TRUE(incr_result.has_value());
                        EXPECT_EQ(incr_result.value(), 2);
                    }

                    {
                        auto decr_result = co_await connection->decr(counter_key);

                        EXPECT_TRUE(decr_result.has_value());
                        EXPECT_EQ(decr_result.value(), 1);
                    }

                    {
                        auto decr_result = co_await connection->decr(counter_key);

                        EXPECT_TRUE(decr_result.has_value());
                        EXPECT_EQ(decr_result.value(), 0);
                    }

                    {
                        auto key_del = co_await connection->del(counter_key);

                        EXPECT_TRUE(key_del);
                    }
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