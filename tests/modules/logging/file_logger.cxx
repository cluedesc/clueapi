#include <gtest/gtest.h>

#include <clueapi.hxx>

std::string
read_file_content(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open())
        return "";

    std::stringstream buffer;

    buffer << file.rdbuf();

    return buffer.str();
}

class file_logger_tests : public ::testing::Test {
  protected:
    void SetUp() override {
        log_path_1 = (std::filesystem::temp_directory_path() / "test_log_1.log").string();

        log_path_2 = (std::filesystem::temp_directory_path() / "test_log_2.log").string();

        std::filesystem::remove(log_path_1);
        std::filesystem::remove(log_path_2);
    }

    void TearDown() override {
        m_logging.destroy();

        std::filesystem::remove(log_path_1);
        std::filesystem::remove(log_path_2);
    }

    std::string log_path_1;
    std::string log_path_2;

    clueapi::modules::logging::c_logging m_logging;
};

class logging_file_sync_tests : public file_logger_tests {
  protected:
    void SetUp() override {
        file_logger_tests::SetUp();

        clueapi::modules::logging::cfg_t cfg{};

        cfg.m_async_mode = false;

        m_logging.init(cfg);
    }
};

TEST_F(logging_file_sync_tests, log_single_message) {
    auto logger = std::make_shared<clueapi::modules::logging::file_logger_t>(clueapi::modules::logging::logger_params_t{
        .m_name = "sync file logger", .m_level = clueapi::modules::logging::e_log_level::info, .m_async_mode = false
    });

    logger->set_file_path(log_path_1);

    m_logging.add_logger(LOGGER_NAME("Test file logger"), logger);

    CLUEAPI_LOG_IMPL(
        m_logging,

        LOGGER_NAME("Test file logger"),

        clueapi::modules::logging::e_log_level::info,

        "Hello, Sync World!"
    );

    std::string content = read_file_content(log_path_1);

    ASSERT_NE(content.find("Hello, Sync World!"), std::string::npos);

    ASSERT_TRUE(std::filesystem::exists(log_path_1));
}

TEST_F(logging_file_sync_tests, log_multiple_messages_and_append) {
    auto logger = std::make_shared<clueapi::modules::logging::file_logger_t>(clueapi::modules::logging::logger_params_t{
        .m_async_mode = false
    });

    logger->set_file_path(log_path_1);

    m_logging.add_logger(LOGGER_NAME("Test file logger"), logger);

    CLUEAPI_LOG_IMPL(
        m_logging,

        LOGGER_NAME("Test file logger"),

        clueapi::modules::logging::e_log_level::warning,

        "First message."
    );

    std::string content1 = read_file_content(log_path_1);

    ASSERT_NE(content1.find("First message."), std::string::npos);

    CLUEAPI_LOG_IMPL(
        m_logging,

        LOGGER_NAME("Test file logger"),

        clueapi::modules::logging::e_log_level::error,

        "Second message."
    );

    std::string content2 = read_file_content(log_path_1);

    ASSERT_NE(content2.find("First message."), std::string::npos);
    ASSERT_NE(content2.find("Second message."), std::string::npos);
}

TEST_F(logging_file_sync_tests, change_file_on_fly) {
    auto logger = std::make_shared<clueapi::modules::logging::file_logger_t>(clueapi::modules::logging::logger_params_t{
        .m_async_mode = false
    });

    logger->set_file_path(log_path_1);

    m_logging.add_logger(LOGGER_NAME("Test file logger"), logger);

    CLUEAPI_LOG_IMPL(
        m_logging,

        LOGGER_NAME("Test file logger"),

        clueapi::modules::logging::e_log_level::error,

        "Logging to file 1."
    );

    logger->set_file_path(log_path_2);

    CLUEAPI_LOG_IMPL(
        m_logging,

        LOGGER_NAME("Test file logger"),

        clueapi::modules::logging::e_log_level::error,

        "Logging to file 2."
    );

    std::string content1 = read_file_content(log_path_1);
    std::string content2 = read_file_content(log_path_2);

    EXPECT_NE(content1.find("Logging to file 1."), std::string::npos);
    EXPECT_EQ(content1.find("Logging to file 2."), std::string::npos);

    EXPECT_EQ(content2.find("Logging to file 1."), std::string::npos);
    EXPECT_NE(content2.find("Logging to file 2."), std::string::npos);
}

class logging_file_async_tests : public file_logger_tests {
  protected:
    void SetUp() override {
        file_logger_tests::SetUp();

        clueapi::modules::logging::cfg_t cfg{};

        cfg.m_async_mode = true;

        cfg.m_sleep = std::chrono::milliseconds{10};

        m_logging.init(cfg);
    }
};

TEST_F(logging_file_async_tests, log_batch_async_messages) {
    auto logger = std::make_shared<clueapi::modules::logging::file_logger_t>(clueapi::modules::logging::logger_params_t{
        .m_name       = "async file logger",
        .m_level      = clueapi::modules::logging::e_log_level::debug,
        .m_capacity   = 100,
        .m_batch_size = 5,
        .m_async_mode = true
    });

    logger->set_file_path(log_path_1);

    m_logging.add_logger(LOGGER_NAME("Test file logger"), logger);

    for (int i = 0; i < 7; ++i) {
        CLUEAPI_LOG_IMPL(
            m_logging,

            LOGGER_NAME("Test file logger"),

            clueapi::modules::logging::e_log_level::error,

            "Async message {}", i
        );
    }

    auto file_logger = m_logging.get_logger(LOGGER_NAME("Test file logger"));

    ASSERT_NE(file_logger, nullptr);
    EXPECT_EQ(file_logger->buffer().size(), 7u);

    std::this_thread::sleep_for(std::chrono::milliseconds{25});

    EXPECT_EQ(file_logger->buffer().size(), 0u);
}

TEST_F(logging_file_async_tests, overflow_handling) {
    auto logger = std::make_shared<clueapi::modules::logging::file_logger_t>(clueapi::modules::logging::logger_params_t{
        .m_capacity = 5, .m_batch_size = 5, .m_async_mode = true
    });

    logger->set_file_path(log_path_1);

    m_logging.add_logger(LOGGER_NAME("Test file logger"), logger);

    for (int i = 0; i < 5; ++i) {
        CLUEAPI_LOG_IMPL(
            m_logging,

            LOGGER_NAME("Test file logger"),

            clueapi::modules::logging::e_log_level::error,

            "Message {}", i
        );
    }

    auto file_logger = m_logging.get_logger(LOGGER_NAME("Test file logger"));

    ASSERT_NE(file_logger, nullptr);
    EXPECT_EQ(file_logger->buffer().size(), 5u);

    CLUEAPI_LOG_IMPL(
        m_logging,

        LOGGER_NAME("Test file logger"),

        clueapi::modules::logging::e_log_level::error,

        "Overflow message"
    );

    EXPECT_EQ(file_logger->buffer().size(), 5u);

    std::this_thread::sleep_for(std::chrono::milliseconds{15});

    EXPECT_EQ(file_logger->buffer().size(), 0u);

    std::string content = read_file_content(log_path_1);

    EXPECT_EQ(content.find("Message 0"), std::string::npos);

    EXPECT_NE(content.find("Message 1"), std::string::npos);
    EXPECT_NE(content.find("Message 4"), std::string::npos);

    EXPECT_NE(content.find("Overflow message"), std::string::npos);
}