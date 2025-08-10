#include <gtest/gtest.h>

#include <clueapi.hxx>

class exceptions_tests : public ::testing::Test {};

TEST_F(exceptions_tests, base_exception_with_default_prefix) {
    using clueapi::exceptions::exception_t;

    exception_t ex("something went wrong");

    EXPECT_STREQ(ex.what(), "something went wrong");
}

TEST_F(exceptions_tests, base_exception_with_formatting) {
    using clueapi::exceptions::exception_t;

    exception_t ex("error code: {}", 404);

    EXPECT_STREQ(ex.what(), "error code: 404");
}

TEST_F(exceptions_tests, custom_exception_types_have_correct_prefixes) {
    using namespace clueapi::exceptions;

    invalid_argument_t inv_arg("null pointer");

    EXPECT_STREQ(inv_arg.what(), "Invalid argument: null pointer");

    runtime_error_t rt_err("process failed");

    EXPECT_STREQ(rt_err.what(), "Runtime error: process failed");

    io_error_t io_err("disk is full");

    EXPECT_STREQ(io_err.what(), "I/O error: disk is full");
}

TEST_F(exceptions_tests, custom_exception_with_formatting) {
    using clueapi::exceptions::invalid_argument_t;

    invalid_argument_t ex("value out of range: {}", 101);

    EXPECT_STREQ(ex.what(), "Invalid argument: value out of range: 101");
}

TEST_F(exceptions_tests, static_make_function_formats_correctly) {
    using clueapi::exceptions::io_error_t;

    auto msg = io_error_t::make("failed to read from socket {}", 5);

    EXPECT_EQ(msg, "I/O error: failed to read from socket 5");
}

class wrap_tests : public ::testing::Test {};

TEST_F(wrap_tests, success_with_return_value) {
    using clueapi::exceptions::wrap;

    auto result = wrap<int>([] { return 42; }, "test_context");

    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result.value(), 42);
}

TEST_F(wrap_tests, success_with_void_return) {
    using clueapi::exceptions::wrap;

    bool called = false;

    auto result = wrap<>([&] { called = true; }, "test_context");

    EXPECT_TRUE(result.has_value());

    EXPECT_TRUE(called);
}

TEST_F(wrap_tests, catches_std_exception) {
    using clueapi::exceptions::wrap;

    auto result = wrap<>([] { throw std::runtime_error("a standard error"); }, "test_context");

    ASSERT_FALSE(result.has_value());

    EXPECT_EQ(result.error(), "test_context: a standard error");
}

TEST_F(wrap_tests, catches_boost_system_error) {
    using clueapi::exceptions::wrap;

    auto result = wrap<>(
        [] {
            throw boost::system::system_error(boost::asio::error::make_error_code(boost::asio::error::connection_refused
            ));
        },

        "network_op"
    );

    ASSERT_FALSE(result.has_value());

    EXPECT_NE(result.error().find("network_op:"), std::string::npos);
    EXPECT_NE(result.error().find("Connection refused"), std::string::npos);
}

TEST_F(wrap_tests, catches_custom_clueapi_exception) {
    using namespace clueapi::exceptions;

    auto result = wrap<>([] { throw exception_t("bad config"); }, "Startup");

    ASSERT_FALSE(result.has_value());

    EXPECT_EQ(result.error(), "Startup: bad config");
}

TEST_F(wrap_tests, catches_unknown_exception) {
    using clueapi::exceptions::wrap;

    auto result = wrap<>([] { throw 123; }, "unknown_source");

    ASSERT_FALSE(result.has_value());

    EXPECT_EQ(result.error(), "unknown_source: unknown");
}

class wrap_awaitable_tests : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
};

TEST_F(wrap_awaitable_tests, success_with_return_value) {
    using namespace clueapi::exceptions;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto awaitable = []() -> boost::asio::awaitable<int> {
                co_return 42;
            };

            auto result = co_await wrap_awaitable<int>(awaitable(), "async_test");

            EXPECT_TRUE(result.has_value());

            EXPECT_EQ(result.value(), 42);
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(wrap_awaitable_tests, success_with_void_return) {
    using namespace clueapi::exceptions;

    bool called = false;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto awaitable = [&]() -> boost::asio::awaitable<void> {
                called = true;

                co_return;
            };

            auto result = co_await wrap_awaitable<void>(awaitable(), "async_void_test");

            EXPECT_TRUE(result.has_value());
        },

        boost::asio::detached
    );

    io_context.run();

    EXPECT_TRUE(called);
}

TEST_F(wrap_awaitable_tests, catches_std_exception) {
    using namespace clueapi::exceptions;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto awaitable = []() -> boost::asio::awaitable<void> {
                throw std::logic_error("async logic fail");

                co_return;
            };

            auto result = co_await wrap_awaitable<void>(awaitable(), "async_fail");

            EXPECT_FALSE(result.has_value());

            EXPECT_EQ(result.error(), "async_fail: async logic fail");
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(wrap_awaitable_tests, catches_boost_system_error) {
    using namespace clueapi::exceptions;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto awaitable = []() -> boost::asio::awaitable<void> {
                throw boost::system::system_error(
                    boost::asio::error::make_error_code(boost::asio::error::connection_refused)
                );

                co_return;
            };

            auto result = co_await wrap_awaitable<void>(awaitable(), "network_op");

            EXPECT_FALSE(result.has_value());

            std::cerr << result.error() << std::endl;

            EXPECT_NE(result.error().find("network_op:"), std::string::npos);
            EXPECT_NE(result.error().find("Connection refused"), std::string::npos);
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(wrap_awaitable_tests, catches_unknown_exception) {
    using namespace clueapi::exceptions;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto awaitable = []() -> boost::asio::awaitable<void> {
                throw "a string literal exception";

                co_return;
            };

            auto result = co_await wrap_awaitable<void>(awaitable(), "async_unknown");

            EXPECT_FALSE(result.has_value());

            EXPECT_EQ(result.error(), "async_unknown: unknown");
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(wrap_awaitable_tests, works_with_invocable_that_returns_awaitable) {
    using namespace clueapi::exceptions;

    boost::asio::co_spawn(
        io_context,
        [&]() -> boost::asio::awaitable<void> {
            auto invocable_factory = [] {
                return []() -> boost::asio::awaitable<std::string> {
                    co_return "invoked";
                };
            };

            auto result = co_await wrap_awaitable<std::string>(invocable_factory(), "invocable_test");

            EXPECT_TRUE(result.has_value());

            EXPECT_EQ(result.value(), "invoked");
        },

        boost::asio::detached
    );

    io_context.run();
}