#include <gtest/gtest.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/error_code.hpp>
#include <exception>
#include <functional>
#include <fstream>
#include <ios>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "clueapi/http/ctx/ctx.hxx"
#include "clueapi/http/types/request/request.hxx"

std::string
create_test_multipart_body(
    const std::string& boundary, const std::map<std::string, std::string>& fields,
    const std::map<std::string, std::pair<std::string, std::string>>& files
) {
    std::stringstream body;

    const std::string crlf = "\r\n";

    for (const auto& field : fields) {
        body << "--" << boundary << crlf;
        body << "Content-Disposition: form-data; name=\"" << field.first << "\"" << crlf << crlf;
        body << field.second << crlf;
    }

    for (const auto& file : files) {
        body << "--" << boundary << crlf;
        body << "Content-Disposition: form-data; name=\"" << file.first << "\"; filename=\"" << file.second.first
             << "\"" << crlf;
        body << "Content-Type: application/octet-stream" << crlf << crlf;
        body << file.second.second << crlf;
    }

    body << "--" << boundary << "--" << crlf;

    return body.str();
}

class http_ctx_tests : public ::testing::Test {
  protected:
    void SetUp() override {
        temp_dir_ = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

        boost::filesystem::create_directory(temp_dir_);

        io_context_.restart();
    }

    void TearDown() override {
        io_context_.stop();

        while (!io_context_.stopped())
            io_context_.poll();

        boost::system::error_code ec;

        boost::filesystem::remove_all(temp_dir_, ec);
    }

    boost::filesystem::path create_temp_file(const std::string& content) {
        auto path = temp_dir_ / boost::filesystem::unique_path();

        std::ofstream file(path.string(), std::ios::binary);

        file << content;

        file.close();

        return path;
    }

    void run_test(std::function<boost::asio::awaitable<void>()> awaitable) {
        boost::asio::co_spawn(
            io_context_,

            std::move(awaitable),
            [this](std::exception_ptr p) {
                if (p) {
                    try {
                        std::rethrow_exception(p);
                    }
                    catch (const std::exception& e) {
                        FAIL() << "Coroutine threw exception: " << e.what();
                    }
                }
            }
        );

        io_context_.run();
    }

    boost::asio::io_context io_context_;

    boost::filesystem::path temp_dir_;

    const std::string boundary_ = "----TestBoundary123";
};

TEST_F(http_ctx_tests, parse_from_memory) {
    using namespace clueapi::http;

    run_test([this]() -> boost::asio::awaitable<void> {
        auto body_str
            = create_test_multipart_body(boundary_, {{"field1", "value1"}}, {{"file1", {"test.txt", "content"}}});

        types::request_t req;

        req.body() = body_str;

        req.headers()["Content-Type"] = "multipart/form-data; boundary=" + boundary_;

        auto ctx = co_await ctx_t::make_awaitable(std::move(req), {}, {});

        EXPECT_EQ(ctx.fields().size(), 1);
        EXPECT_EQ(ctx.fields().at("field1"), "value1");
        EXPECT_EQ(ctx.files().size(), 1);
        EXPECT_EQ(ctx.files().at("file1").name(), "test.txt");

        EXPECT_TRUE(ctx.files().at("file1").in_memory());
    });
}

TEST_F(http_ctx_tests, parse_from_file_and_delete_files) {
    using namespace clueapi::http;

    run_test([this]() -> boost::asio::awaitable<void> {
        auto body_str = create_test_multipart_body(boundary_, {{"field_from_file", "val"}}, {});

        auto temp_file_path = create_temp_file(body_str);

        EXPECT_TRUE(boost::filesystem::exists(temp_file_path));

        types::request_t req;

        req.parse_path() = temp_file_path;

        req.headers()["Content-Type"] = "multipart/form-data; boundary=" + boundary_;

        auto ctx = co_await ctx_t::make_awaitable(std::move(req), {}, {});

        EXPECT_EQ(ctx.fields().size(), 1);
        EXPECT_EQ(ctx.fields().at("field_from_file"), "val");

        EXPECT_TRUE(ctx.files().empty());

        EXPECT_FALSE(boost::filesystem::exists(temp_file_path));
    });
}

TEST_F(http_ctx_tests, non_multipart_request_is_ignored) {
    using namespace clueapi::http;

    run_test([this]() -> boost::asio::awaitable<void> {
        types::request_t req;

        req.body()                    = "this is not multipart data";
        req.headers()["Content-Type"] = "application/json";

        auto ctx = co_await ctx_t::make_awaitable(std::move(req), {}, {});

        EXPECT_TRUE(ctx.fields().empty());
        EXPECT_TRUE(ctx.files().empty());
    });
}

TEST_F(http_ctx_tests, multipart_request_with_no_boundary_is_ignored) {
    using namespace clueapi::http;

    run_test([this]() -> boost::asio::awaitable<void> {
        types::request_t req;

        req.body() = "some data";

        req.headers()["Content-Type"] = "multipart/form-data";

        auto ctx = co_await ctx_t::make_awaitable(std::move(req), {}, {});

        EXPECT_TRUE(ctx.fields().empty());
        EXPECT_TRUE(ctx.files().empty());
    });
}

TEST_F(http_ctx_tests, malformed_body_handled_gracefully) {
    using namespace clueapi::http;

    run_test([this]() -> boost::asio::awaitable<void> {
        std::string malformed_body
            = "--" + boundary_ + "\r\n" + "Content-Disposition: form-data; name=\"field\"\r\n\r\n" + "value\r\n";

        types::request_t req;

        req.body() = malformed_body;

        req.headers()["Content-Type"] = "multipart/form-data; boundary=" + boundary_;

        auto ctx = co_await ctx_t::make_awaitable(std::move(req), {}, {});

        EXPECT_TRUE(ctx.fields().empty());
        EXPECT_TRUE(ctx.files().empty());
    });
}