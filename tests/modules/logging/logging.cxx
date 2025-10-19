#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "clueapi/modules/logging/detail/hash/hash.hxx"
#include "clueapi/modules/logging/loggers/console/console.hxx"
#include "clueapi/modules/logging/logging.hxx"

class logging_sync_tests : public ::testing::Test {
  protected:
    void SetUp() override {
        clueapi::modules::logging::cfg_t cfg{};

        cfg.m_async_mode = false;
        cfg.m_sleep      = std::chrono::milliseconds{0};

        m_logging.init(cfg);
    }

    void TearDown() override { m_logging.destroy(); }

    clueapi::modules::logging::c_logging m_logging;
};

class logging_async_tests : public ::testing::Test {
  protected:
    void SetUp() override {
        clueapi::modules::logging::cfg_t cfg{};

        cfg.m_async_mode = true;
        cfg.m_sleep      = std::chrono::milliseconds{5};

        m_logging.init(cfg);
    }

    void TearDown() override { m_logging.destroy(); }

    clueapi::modules::logging::c_logging m_logging;
};

TEST_F(logging_sync_tests, add_sync_logger) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "sync console logger",
            .m_level      = clueapi::modules::logging::e_log_level::info,
            .m_capacity   = 256,
            .m_batch_size = 10,
            .m_async_mode = false
        });

    m_logging.add_logger(LOGGER_NAME("Test console logger"), std::move(logger));

    EXPECT_TRUE(m_logging.is_running());

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test console logger"));

    ASSERT_NE(console_logger, nullptr);

    EXPECT_TRUE(console_logger->enabled());

    EXPECT_EQ(console_logger->name(), "sync console logger");

    EXPECT_EQ(console_logger->level(), clueapi::modules::logging::e_log_level::info);

    EXPECT_FALSE(console_logger->async_mode());

    auto& buffer = console_logger->buffer();

    EXPECT_EQ(buffer.size(), 0u);

    EXPECT_EQ(buffer.capacity(), 256u);

    auto& params = console_logger->params();

    EXPECT_EQ(params.m_async_mode, false);

    EXPECT_EQ(params.m_batch_size, 10u);

    EXPECT_EQ(params.m_capacity, 256u);

    EXPECT_EQ(params.m_level, clueapi::modules::logging::e_log_level::info);

    EXPECT_EQ(params.m_name, "sync console logger");
}

TEST_F(logging_sync_tests, remove_sync_logger) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "sync console logger",
            .m_level      = clueapi::modules::logging::e_log_level::info,
            .m_capacity   = 256,
            .m_batch_size = 10,
            .m_async_mode = false
        });

    m_logging.add_logger(LOGGER_NAME("Test console logger"), std::move(logger));

    EXPECT_TRUE(m_logging.is_running());

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test console logger"));

    ASSERT_NE(console_logger, nullptr);

    EXPECT_TRUE(console_logger->enabled());

    m_logging.remove_logger(LOGGER_NAME("Test console logger"));

    ASSERT_EQ(m_logging.get_logger(LOGGER_NAME("Test console logger")), nullptr);
}

TEST_F(logging_sync_tests, test_messages_sync_logger) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "sync console logger",
            .m_level      = clueapi::modules::logging::e_log_level::info,
            .m_capacity   = 256,
            .m_batch_size = 10,
            .m_async_mode = false
        });

    m_logging.add_logger(LOGGER_NAME("Test console logger"), std::move(logger));

    EXPECT_TRUE(m_logging.is_running());

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test console logger"));

    ASSERT_NE(console_logger, nullptr);

    EXPECT_TRUE(console_logger->enabled());

    m_logging.remove_logger(LOGGER_NAME("Test console logger"));

    ASSERT_EQ(m_logging.get_logger(LOGGER_NAME("Test console logger")), nullptr);

    std::string msg = "test message";

    console_logger->log(clueapi::modules::logging::log_msg_t{
        .m_msg   = std::move(msg),
        .m_level = clueapi::modules::logging::e_log_level::info,
        .m_time  = std::chrono::system_clock::now()
    });

    m_logging.remove_logger(LOGGER_NAME("Test console logger"));

    ASSERT_EQ(m_logging.get_logger(LOGGER_NAME("Test console logger")), nullptr);
}

TEST_F(logging_async_tests, add_async_logger) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "async console logger",
            .m_level      = clueapi::modules::logging::e_log_level::debug,
            .m_capacity   = 512,
            .m_batch_size = 20,
            .m_async_mode = true
        });

    m_logging.add_logger(LOGGER_NAME("Test async logger"), std::move(logger));

    EXPECT_TRUE(m_logging.is_running());

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test async logger"));

    ASSERT_NE(console_logger, nullptr);

    EXPECT_TRUE(console_logger->enabled());

    EXPECT_EQ(console_logger->name(), "async console logger");

    EXPECT_EQ(console_logger->level(), clueapi::modules::logging::e_log_level::debug);

    EXPECT_TRUE(console_logger->async_mode());

    auto& buffer = console_logger->buffer();

    EXPECT_EQ(buffer.size(), 0u);

    EXPECT_EQ(buffer.capacity(), 512u);

    auto& params = console_logger->params();

    EXPECT_TRUE(params.m_async_mode);

    EXPECT_EQ(params.m_batch_size, 20u);

    EXPECT_EQ(params.m_capacity, 512u);

    EXPECT_EQ(params.m_level, clueapi::modules::logging::e_log_level::debug);

    EXPECT_EQ(params.m_name, "async console logger");
}

TEST_F(logging_async_tests, remove_async_logger) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name = "async console logger", .m_async_mode = true
        });

    m_logging.add_logger(LOGGER_NAME("Test async logger"), std::move(logger));

    EXPECT_TRUE(m_logging.is_running());

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test async logger"));

    ASSERT_NE(console_logger, nullptr);

    m_logging.remove_logger(LOGGER_NAME("Test async logger"));

    ASSERT_EQ(m_logging.get_logger(LOGGER_NAME("Test async logger")), nullptr);
}

#ifndef _WIN32
TEST_F(logging_async_tests, test_messages_async_logger) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "async console logger",
            .m_level      = clueapi::modules::logging::e_log_level::info,
            .m_capacity   = 256,
            .m_batch_size = 10,
            .m_async_mode = true
        });

    m_logging.add_logger(LOGGER_NAME("Test async logger"), std::move(logger));

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test async logger"));

    ASSERT_NE(console_logger, nullptr);

    for (int i = 0; i < 5; ++i) {
        console_logger->log(clueapi::modules::logging::log_msg_t{
            .m_msg   = "test message " + std::to_string(i),
            .m_level = clueapi::modules::logging::e_log_level::info,
            .m_time  = std::chrono::system_clock::now()
        });
    }

    EXPECT_EQ(console_logger->buffer().size(), 5u);

    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    EXPECT_EQ(console_logger->buffer().size(), 0u);
}
#endif // _WIN32

TEST_F(logging_async_tests, test_batch_processing_async_logger) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "async console logger",
            .m_level      = clueapi::modules::logging::e_log_level::info,
            .m_capacity   = 50,
            .m_batch_size = 10,
            .m_async_mode = true
        });

    m_logging.add_logger(LOGGER_NAME("Test async logger"), std::move(logger));

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test async logger"));

    ASSERT_NE(console_logger, nullptr);

    for (int i = 0; i < 15; ++i) {
        console_logger->log(clueapi::modules::logging::log_msg_t{
            .m_msg   = "test message " + std::to_string(i),
            .m_level = clueapi::modules::logging::e_log_level::info,
            .m_time  = std::chrono::system_clock::now()
        });
    }

    EXPECT_EQ(console_logger->buffer().size(), 15u);

    std::this_thread::sleep_for(std::chrono::milliseconds{25});

    EXPECT_EQ(console_logger->buffer().size(), 0u);
}

TEST_F(logging_async_tests, test_mixed_loggers) {
    auto async_logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "async logger",
            .m_level      = clueapi::modules::logging::e_log_level::info,
            .m_capacity   = 256,
            .m_batch_size = 5,
            .m_async_mode = true
        });

    m_logging.add_logger(LOGGER_NAME("async"), std::move(async_logger));

    auto sync_logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name = "sync logger", .m_level = clueapi::modules::logging::e_log_level::info, .m_async_mode = false
        });

    m_logging.add_logger(LOGGER_NAME("sync"), std::move(sync_logger));

    auto logger_async = m_logging.get_logger(LOGGER_NAME("async"));

    ASSERT_NE(logger_async, nullptr);

    auto logger_sync = m_logging.get_logger(LOGGER_NAME("sync"));

    ASSERT_NE(logger_sync, nullptr);

    logger_sync->log(clueapi::modules::logging::log_msg_t{
        .m_msg   = "sync message",
        .m_level = clueapi::modules::logging::e_log_level::info,
        .m_time  = std::chrono::system_clock::now()
    });

    EXPECT_EQ(logger_sync->buffer().size(), 0u);

    logger_async->log(clueapi::modules::logging::log_msg_t{
        .m_msg   = "async message 1",
        .m_level = clueapi::modules::logging::e_log_level::info,
        .m_time  = std::chrono::system_clock::now()
    });

    logger_async->log(clueapi::modules::logging::log_msg_t{
        .m_msg   = "async message 2",
        .m_level = clueapi::modules::logging::e_log_level::info,
        .m_time  = std::chrono::system_clock::now()
    });

    EXPECT_EQ(logger_async->buffer().size(), 2u);

    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    EXPECT_EQ(logger_async->buffer().size(), 0u);
}

TEST_F(logging_async_tests, test_log_macros) {
    auto logger
        = std::make_shared<clueapi::modules::logging::console_logger_t>(clueapi::modules::logging::logger_params_t{
            .m_name       = "sync console logger",
            .m_level      = clueapi::modules::logging::e_log_level::info,
            .m_capacity   = 256,
            .m_batch_size = 10,
            .m_async_mode = false
        });

    m_logging.add_logger(LOGGER_NAME("Test console logger"), std::move(logger));

    EXPECT_TRUE(m_logging.is_running());

    auto console_logger = m_logging.get_logger(LOGGER_NAME("Test console logger"));

    ASSERT_NE(console_logger, nullptr);

    EXPECT_TRUE(console_logger->enabled());

    CLUEAPI_LOG_IMPL(
        m_logging,

        LOGGER_NAME("Test console logger"),

        clueapi::modules::logging::e_log_level::info,

        "test message {} {}", 1, 2
    );

    auto* logging_ptr = &m_logging;

    CLUEAPI_LOG_IMPL(
        logging_ptr,

        LOGGER_NAME("Test console logger"),

        clueapi::modules::logging::e_log_level::info,

        "test message {} {}", 1, 2
    );
}