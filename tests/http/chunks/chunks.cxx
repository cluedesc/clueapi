#include <gtest/gtest.h>

#include <array>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string>

#include "clueapi/exceptions/wrap/wrap.hxx"
#include "clueapi/http/chunks/chunks.hxx"

#define EXPECT_EXPECTED(expr)                                                                                          \
    {                                                                                                                  \
        auto result = (expr);                                                                                          \
        EXPECT_TRUE(result.has_value());                                                                               \
    }

#define EXCEPT_UNEXPECTED(expr)                                                                                        \
    {                                                                                                                  \
        auto result = (expr);                                                                                          \
        EXPECT_FALSE(result.has_value());                                                                              \
    }

#define EXPECT_EXPECTED_AWAITABLE(expr)                                                                                \
    {                                                                                                                  \
        auto result = (expr);                                                                                          \
        EXPECT_TRUE(result.has_value());                                                                               \
    }

#define EXPECT_UNEXPECTED(expr)                                                                                        \
    {                                                                                                                  \
        auto result = (expr);                                                                                          \
        EXPECT_FALSE(result.has_value());                                                                              \
    }

class chunk_writer_tests : public ::testing::Test {
  protected:
    void SetUp() override {
        auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0);

        acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(io_context, endpoint);

        client_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_context);
        server_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_context);

        acceptor->async_accept(*server_socket, [this](const boost::system::error_code& ec) { ASSERT_FALSE(ec); });

        client_socket->async_connect(acceptor->local_endpoint(), [this](const boost::system::error_code& ec) {
            ASSERT_FALSE(ec);
        });

        io_context.run();

        io_context.restart();
    }

    void TearDown() override {
        if (client_socket->is_open())
            client_socket->close();

        if (server_socket->is_open())
            server_socket->close();

        if (acceptor->is_open())
            acceptor->close();
    }

    template <size_t N>
    boost::asio::awaitable<std::string> read_from_socket() {
        std::array<char, N> buffer{};

        const auto bytes_read = co_await server_socket->async_read_some(
            boost::asio::buffer(buffer),

            boost::asio::use_awaitable
        );

        co_return std::string(buffer.data(), bytes_read);
    }

    boost::asio::io_context io_context;

    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    std::unique_ptr<boost::asio::ip::tcp::socket>   client_socket;
    std::unique_ptr<boost::asio::ip::tcp::socket>   server_socket;
};

TEST_F(chunk_writer_tests, initial_state) {
    using clueapi::http::chunks::chunk_writer_t;

    chunk_writer_t writer(*client_socket);

    EXPECT_FALSE(writer.final_chunk_written());
    EXPECT_FALSE(writer.writer_closed());
}

TEST_F(chunk_writer_tests, write_single_chunk) {
    using clueapi::http::chunks::chunk_writer_t;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            chunk_writer_t writer(*client_socket);

            EXPECT_EXPECTED(co_await writer.write_chunk("test data"));

            co_return;
        },

        boost::asio::detached
    );

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto result = co_await read_from_socket<128>();

            EXPECT_EQ(result, "9\r\ntest data\r\n");

            co_return;
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(chunk_writer_tests, write_multiple_chunks) {
    using clueapi::http::chunks::chunk_writer_t;

    boost::asio::co_spawn(
        io_context,
        [&]() -> boost::asio::awaitable<void> {
            chunk_writer_t writer(*client_socket);

            EXPECT_EXPECTED(co_await writer.write_chunk("chunk1"));

            EXPECT_EXPECTED(co_await writer.write_chunk("another chunk"));

            co_return;
        },

        boost::asio::detached
    );

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto result1 = co_await read_from_socket<128>();

            EXPECT_EQ(result1, "6\r\nchunk1\r\n");

            auto result2 = co_await read_from_socket<128>();

            EXPECT_EQ(result2, "D\r\nanother chunk\r\n");

            co_return;
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(chunk_writer_tests, write_empty_chunk) {
    using clueapi::http::chunks::chunk_writer_t;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            chunk_writer_t writer(*client_socket);

            EXPECT_EXPECTED(co_await writer.write_chunk(""));

            co_return;
        },

        boost::asio::detached
    );

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto result = co_await read_from_socket<32>();

            EXPECT_EQ(result, "0\r\n\r\n");

            co_return;
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(chunk_writer_tests, write_final_chunk) {
    using clueapi::http::chunks::chunk_writer_t;

    chunk_writer_t writer(*client_socket);

    ASSERT_FALSE(writer.final_chunk_written());

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            EXPECT_EXPECTED(co_await writer.write_final_chunk());

            co_return;
        },

        boost::asio::detached
    );

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto result = co_await read_from_socket<32>();

            EXPECT_EQ(result, "0\r\n\r\n");

            co_return;
        },

        boost::asio::detached
    );

    io_context.run();

    EXPECT_TRUE(writer.final_chunk_written());
}

TEST_F(chunk_writer_tests, write_final_chunk_is_idempotent) {
    using clueapi::http::chunks::chunk_writer_t;

    chunk_writer_t writer(*client_socket);

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            EXPECT_EXPECTED(co_await writer.write_final_chunk());

            EXPECT_EXPECTED(co_await writer.write_final_chunk());

            co_return;
        },

        boost::asio::detached
    );

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto result = co_await read_from_socket<64>();

            EXPECT_EQ(result, "0\r\n\r\n");
            EXPECT_EQ(result.size(), 5);

            co_return;
        },

        boost::asio::detached
    );

    io_context.run();

    EXPECT_TRUE(writer.final_chunk_written());
}

TEST_F(chunk_writer_tests, write_after_final_chunk) {
    using clueapi::http::chunks::chunk_writer_t;

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            chunk_writer_t writer(*client_socket);

            EXPECT_EXPECTED(co_await writer.write_final_chunk());

            EXPECT_TRUE(writer.final_chunk_written());

            EXPECT_EXPECTED(co_await writer.write_chunk("extra data"));

            co_return;
        },

        boost::asio::detached
    );

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto final_chunk = co_await read_from_socket<32>();

            EXPECT_EQ(final_chunk, "0\r\n\r\n");

            auto extra_chunk = co_await read_from_socket<128>();

            EXPECT_EQ(extra_chunk, "A\r\nextra data\r\n");

            co_return;
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(chunk_writer_tests, write_fails_on_closed_socket) {
    using clueapi::http::chunks::chunk_writer_t;

    chunk_writer_t writer(*client_socket);

    client_socket->close();

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto result = co_await writer.write_chunk("this should fail");

            EXPECT_FALSE(result.has_value());

            if (!result.has_value())
                EXPECT_NE(result.error().find("Failed to write chunk"), std::string::npos);
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(chunk_writer_tests, writer_closed_reflects_socket_state) {
    using clueapi::http::chunks::chunk_writer_t;

    chunk_writer_t writer(*client_socket);

    EXPECT_FALSE(writer.writer_closed());

    client_socket->close();

    EXPECT_TRUE(writer.writer_closed());
}