#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "clueapi/clueapi.hxx"
#include "clueapi/exceptions/wrap/wrap.hxx"
#include "clueapi/http/chunks/chunks.hxx"
#include "clueapi/http/ctx/ctx.hxx"
#include "clueapi/http/types/method/method.hxx"
#include "clueapi/http/types/response/response.hxx"
#include "clueapi/http/types/status/status.hxx"
#include "clueapi/middleware/middleware.hxx"
#include "clueapi/shared/shared.hxx"

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;

using tcp = asio::ip::tcp;

class add_header_middleware : public clueapi::middleware::c_base_middleware {
   public:
    add_header_middleware(std::string header, std::string value)
        : header_(std::move(header)), value_(std::move(value)) {
    }

    clueapi::shared::awaitable_t<clueapi::http::types::response_t> handle(
        const clueapi::http::types::request_t& request,

        clueapi::middleware::next_t next) override {
        auto response = co_await next(request);

        response.headers()[header_] = value_;

        co_return response;
    }

   private:
    std::string header_;

    std::string value_;
};

class clueapi_valid_tests : public ::testing::Test {
   protected:
    void SetUp() override {
        clueapi::cfg_t cfg{};

        {
            cfg.m_host = "127.0.0.1";
            cfg.m_port = "8080";

            cfg.m_workers = 2;

            cfg.m_server.m_acceptor.m_max_connections = 16;

            cfg.m_server.m_acceptor.m_reuse_port = true;
            cfg.m_server.m_acceptor.m_reuse_address = true;

            cfg.m_http.m_keep_alive_enabled = false;

            cfg.m_socket.m_timeout = std::chrono::seconds{5};

#ifdef CLUEAPI_USE_LOGGING_MODULE
            cfg.m_logging_cfg.m_default_level = clueapi::modules::logging::e_log_level::off;
#endif // CLUEAPI_USE_LOGGING_MODULE
        }

        {
            api.add_method(
                clueapi::http::types::method_t::get,
                "/hello",
                [](clueapi::http::ctx_t) -> clueapi::http::types::response_t {
                    return {"Hello, World!", clueapi::http::types::status_t::ok};
                });

            api.add_method(
                clueapi::http::types::method_t::get,
                "/users/{id}/posts/{postId}",
                [](clueapi::http::ctx_t ctx) -> clueapi::http::types::response_t {
                    auto user_id = ctx.params().at("id");
                    auto post_id = ctx.params().at("postId");

                    return {
                        fmt::format("User: {}, Post: {}", user_id, post_id),
                        clueapi::http::types::status_t::ok};
                });

            api.add_method(
                clueapi::http::types::method_t::post,
                "/echo",
                [](clueapi::http::ctx_t ctx) -> clueapi::http::types::response_t {
                    return {ctx.request().body(), clueapi::http::types::status_t::ok};
                });

            api.add_method(
                clueapi::http::types::method_t::get,
                "/json",
                [](clueapi::http::ctx_t) -> clueapi::http::types::json_response_t {
                    return {
                        clueapi::http::types::json_t::json_obj_t{{"status", "ok"}, {"code", 200}},
                        clueapi::http::types::status_t::ok};
                });

            api.add_method(
                clueapi::http::types::method_t::get,
                "/middleware-test",
                [](clueapi::http::ctx_t) -> clueapi::http::types::response_t {
                    return {"Middleware test", clueapi::http::types::status_t::ok};
                });

            api.add_method(
                clueapi::http::types::method_t::get,
                "/stream",
                [](clueapi::http::ctx_t) -> clueapi::http::types::stream_response_t {
                    return {
                        [](clueapi::http::chunks::chunk_writer_t& writer)
                            -> clueapi::exceptions::expected_awaitable_t<> {
                            auto result = co_await writer.write_chunk("chunk1-");

                            if (!result.has_value())
                                co_return clueapi::exceptions::make_unexpected(result.error());

                            result = co_await writer.write_chunk("part2-");

                            if (!result.has_value())
                                co_return clueapi::exceptions::make_unexpected(result.error());

                            result = co_await writer.write_chunk("final");

                            if (!result.has_value())
                                co_return clueapi::exceptions::make_unexpected(result.error());

                            co_return clueapi::exceptions::expected_t<>{};
                        },
                        "text/plain"};
                });

            api.add_middleware(
                std::make_shared<add_header_middleware>("X-Middleware-Handled", "true"));
        }

        try {
            api.start(cfg);

            std::size_t attempts{};

            while (!api.is_running()) {
                attempts++;

                if (attempts >= 5)
                    FAIL() << "Failed to start API";

                std::this_thread::sleep_for(std::chrono::milliseconds{25});
            }

            ASSERT_TRUE(api.is_running());
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to start API: " << e.what();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    void TearDown() override {
        try {
            api.stop();

            std::size_t attempts{};

            while (!api.is_stopped()) {
                attempts++;

                if (attempts >= 5)
                    FAIL() << "Failed to stop API";

                std::this_thread::sleep_for(std::chrono::milliseconds{50});
            }

            ASSERT_TRUE(api.is_stopped());
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to stop API: " << e.what();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    http::response<http::string_body> perform_request(
        http::verb method, const std::string& target, const std::string& body = "") {
        try {
            asio::io_context ioc;

            tcp::resolver resolver(ioc);

            beast::tcp_stream stream(ioc);

            auto const results = resolver.resolve("127.0.0.1", "8080");

            stream.connect(results);

            stream.expires_after(std::chrono::seconds(1));

            http::request<http::string_body> req{method, target, 11};

            req.set(http::field::host, "127.0.0.1");
            req.set(http::field::connection, "close");

            if (!body.empty()) {
                req.body() = body;
            }

            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;

            http::response<http::string_body> res;

            http::read(stream, buffer, res);

            beast::error_code ec;

            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            stream.socket().cancel(ec);

            stream.socket().close(ec);

            return res;
        } catch (const std::exception& e) {
            ADD_FAILURE() << "Request to target '" << target << "' failed: " << e.what();

            return {};
        }
    }

    clueapi::c_clueapi api{};
};

TEST_F(clueapi_valid_tests, handles_simple_get_request) {
    auto res = perform_request(http::verb::get, "/hello");

    ASSERT_EQ(res.result(), http::status::ok);

    ASSERT_EQ(res.body(), "Hello, World!");

    ASSERT_EQ(res[http::field::content_type], "text/plain");
}

TEST_F(clueapi_valid_tests, handles_url_params) {
    auto res = perform_request(http::verb::get, "/users/123/posts/abc");

    ASSERT_EQ(res.result(), http::status::ok);

    ASSERT_EQ(res.body(), "User: 123, Post: abc");
}

TEST_F(clueapi_valid_tests, handles_post_request_with_body) {
    const std::string request_body = "this is the post body";

    auto res = perform_request(http::verb::post, "/echo", request_body);

    ASSERT_EQ(res.result(), http::status::ok);
    ASSERT_EQ(res.body(), request_body);
}

TEST_F(clueapi_valid_tests, handles_json_response) {
    auto res = perform_request(http::verb::get, "/json");

    ASSERT_EQ(res.result(), http::status::ok);

    ASSERT_EQ(res[http::field::content_type], "application/json");

    ASSERT_EQ(res.body(), R"({"code":200,"status":"ok"})");
}

TEST_F(clueapi_valid_tests, returns_not_found_for_unknown_route) {
    auto res = perform_request(http::verb::get, "/this/route/does/not/exist");

    ASSERT_EQ(res.result(), http::status::not_found);
}

TEST_F(clueapi_valid_tests, middleware_added_header) {
    auto res = perform_request(http::verb::get, "/middleware-test");

    ASSERT_EQ(res.result(), http::status::ok);

    ASSERT_TRUE(res.find("X-Middleware-Handled") != res.end());

    ASSERT_EQ(res["X-Middleware-Handled"], "true");
}

TEST_F(clueapi_valid_tests, handles_chunked_response) {
    auto res = perform_request(http::verb::get, "/stream");

    ASSERT_EQ(res.result(), http::status::ok);

    ASSERT_EQ(res[http::field::transfer_encoding], "chunked");

    ASSERT_EQ(res.body(), "chunk1-part2-final");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}