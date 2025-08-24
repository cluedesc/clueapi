#include <gtest/gtest.h>

#include <clueapi.hxx>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/filesystem.hpp>

using namespace clueapi::http;
using namespace clueapi::http::types;

class response_tests : public ::testing::Test {};

TEST_F(response_tests, base_response_default_constructor) {
    base_response_t res;

    EXPECT_EQ(res.status(), status_t::e_status{});

    EXPECT_TRUE(res.body().empty());
    EXPECT_TRUE(res.headers().empty());

    EXPECT_FALSE(res.is_stream());
}

TEST_F(response_tests, text_response_constructor) {
    text_response_t res("Hello", status_t::ok, {{"X-Custom", "Value"}});

    EXPECT_EQ(res.body(), "Hello");
    EXPECT_EQ(res.status(), status_t::ok);
    EXPECT_EQ(res.headers().at("Content-Type"), "text/plain");
    EXPECT_EQ(res.headers().at("X-Custom"), "Value");
}

TEST_F(response_tests, html_response_sets_correct_content_type) {
    html_response_t res("<h1>Hi</h1>");

    EXPECT_EQ(res.headers().at("Content-Type"), "text/html");
    EXPECT_EQ(res.status(), status_t::ok);
}

TEST_F(response_tests, json_response_sets_content_type_and_body) {
    json_response_t res(R"({"a":1})", status_t::created);

    EXPECT_EQ(res.headers().at("Content-Type"), "application/json");
    EXPECT_EQ(res.body(), "\"{\\\"a\\\":1}\"");
    EXPECT_EQ(res.status(), status_t::created);
}

TEST_F(response_tests, json_response_with_empty_body) {
    json_response_t res(json_t::json_obj_t{}, status_t::ok);

    EXPECT_EQ(res.headers().at("Content-Type"), "application/json");

    EXPECT_EQ(res.body(), "{}");
}

TEST_F(response_tests, redirect_response_with_valid_status) {
    redirect_response_t res("/new-location", status_t::moved_permanently);

    EXPECT_TRUE(res.body().empty());

    EXPECT_EQ(res.status(), status_t::moved_permanently);
    EXPECT_EQ(res.headers().at("Location"), "/new-location");
}

TEST_F(response_tests, redirect_response_defaults_invalid_status_to_found) {
    redirect_response_t res("/another-place", status_t::ok);

    EXPECT_EQ(res.status(), status_t::found);
    EXPECT_EQ(res.headers().at("Location"), "/another-place");
}

TEST_F(response_tests, stream_response_constructor) {
    using namespace clueapi::exceptions;

    stream_response_t res(
        [&](chunks::chunk_writer_t&) -> expected_awaitable_t<> { co_return expected_t<>{}; },

        "video/mp4",

        status_t::ok,

        {{"X-Stream", "true"}});

    EXPECT_TRUE(res.is_stream());

    EXPECT_EQ(res.status(), status_t::ok);

    EXPECT_TRUE(res.body().empty());

    EXPECT_EQ(res.headers().at("Content-Type"), "video/mp4");
    EXPECT_EQ(res.headers().at("Cache-Control"), "no-cache");
    EXPECT_EQ(res.headers().at("X-Stream"), "true");

    ASSERT_TRUE(res.stream_fn());
}

TEST_F(response_tests, set_cookie) {
    text_response_t res("body");

    cookie_t c{
        "sid",
        "12345",
    };

    c.max_age() = std::chrono::seconds{3600};

    auto cookie_set = res.set_cookie(c);

    EXPECT_TRUE(cookie_set.has_value());

    ASSERT_EQ(res.cookies().size(), 1);

    EXPECT_NE(res.cookies()[0].find("sid=12345"), std::string::npos);
    EXPECT_NE(res.cookies()[0].find("Max-Age=3600"), std::string::npos);
}

class file_response_tests : public ::testing::Test {
   protected:
    void SetUp() override {
        temp_dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

        boost::filesystem::create_directory(temp_dir);

        test_file = temp_dir / "test.txt";

        std::ofstream ofs(test_file.string());

        ofs << "hello from file";

        ofs.close();

        auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0);

        acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(io_context, endpoint);

        client_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_context);
        server_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_context);

        acceptor->async_accept(*server_socket, [this](const boost::system::error_code& ec) {
            ASSERT_FALSE(ec) << ec.message();
        });

        client_socket->async_connect(
            acceptor->local_endpoint(),
            [this](const boost::system::error_code& ec) { ASSERT_FALSE(ec) << ec.message(); });

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

        boost::filesystem::remove_all(temp_dir);
    }

    template <size_t _size_t>
    boost::asio::awaitable<std::string> read_from_socket() {
        std::array<char, _size_t> buffer{};

        const auto bytes_read = co_await server_socket->async_read_some(
            boost::asio::buffer(buffer),

            boost::asio::use_awaitable);

        co_return std::string(buffer.data(), bytes_read);
    }

    boost::filesystem::path temp_dir;
    boost::filesystem::path test_file;

    boost::asio::io_context io_context;

    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;

    std::unique_ptr<boost::asio::ip::tcp::socket> client_socket;
    std::unique_ptr<boost::asio::ip::tcp::socket> server_socket;
};

TEST_F(file_response_tests, for_existing_file) {
    file_response_t res(test_file);

    const auto expected_size = boost::filesystem::file_size(test_file);
    const auto last_write = boost::filesystem::last_write_time(test_file);

    const auto expected_etag =
        fmt::format("\"{}\"", fmt::format("{}-{}", last_write, expected_size));

    EXPECT_EQ(res.status(), status_t::ok);

    EXPECT_TRUE(res.is_stream());
    EXPECT_TRUE(res.body().empty());
    EXPECT_TRUE(res.stream_fn());

    EXPECT_EQ(res.headers().at("Content-Type"), "text/plain");
    EXPECT_EQ(res.headers().at("Content-Length"), std::to_string(expected_size));
    EXPECT_EQ(res.headers().at("ETag"), expected_etag);
}

TEST_F(file_response_tests, for_non_existent_file) {
    file_response_t res(temp_dir / "nonexistent.file");

    EXPECT_EQ(res.status(), status_t::not_found);

    EXPECT_FALSE(res.is_stream());
    EXPECT_FALSE(res.stream_fn());

    EXPECT_TRUE(res.headers().empty());
}

TEST_F(file_response_tests, for_directory) {
    file_response_t res(temp_dir);

    EXPECT_EQ(res.status(), status_t::not_found);

    EXPECT_FALSE(res.is_stream());
    EXPECT_FALSE(res.stream_fn());

    EXPECT_TRUE(res.headers().empty());
}

TEST_F(file_response_tests, stream_function_sends_correct_data) {
    file_response_t res(test_file);

    ASSERT_TRUE(res.is_stream());
    ASSERT_TRUE(res.stream_fn());

    co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            chunks::chunk_writer_t writer(*client_socket);

            auto result = co_await res.stream_fn()(writer);

            EXPECT_TRUE(result.has_value());

            result = co_await writer.write_final_chunk();

            EXPECT_TRUE(result.has_value());
        },

        boost::asio::detached);

    co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto result = co_await read_from_socket<256>();

            EXPECT_EQ(result, "F\r\nhello from file\r\n0\r\n\r\n");
        },

        boost::asio::detached);
}

TEST_F(file_response_tests, handles_initialization_error) {
    using namespace clueapi::exceptions;

    auto original_perms = boost::filesystem::status(temp_dir).permissions();

    boost::filesystem::permissions(temp_dir, boost::filesystem::perms::owner_write);

    EXPECT_THROW(file_response_t res(test_file), exception_t);

    boost::filesystem::permissions(temp_dir, original_perms);
}

TEST_F(file_response_tests, handles_io_error_during_streaming) {
    using namespace clueapi::exceptions;

    file_response_t res(test_file);

    ASSERT_TRUE(res.is_stream());
    ASSERT_TRUE(res.stream_fn());

    co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            client_socket->close();

            chunks::chunk_writer_t writer(*client_socket);

            auto result = co_await res.stream_fn()(writer);

            EXPECT_FALSE(result.has_value());

            if (!result.has_value())
                EXPECT_NE(result.error().find("Failed to write chunk"), std::string::npos);
        },

        boost::asio::detached);
}