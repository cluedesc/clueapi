#include <gtest/gtest.h>

#include <clueapi.hxx>

using namespace clueapi;
using namespace clueapi::http;
using namespace clueapi::middleware;

class add_header_middleware : public c_base_middleware {
  public:
    add_header_middleware(std::string header, std::string value)
        : header_(std::move(header)), value_(std::move(value)) {}

    shared::awaitable_t<types::response_t> handle(
        const types::request_t& request,

        next_t next
    ) override {
        auto response = co_await next(request);

        response.headers()[header_] = value_;

        co_return response;
    }

  private:
    std::string header_;

    std::string value_;
};

class short_circuit_middleware : public c_base_middleware {
  public:
    shared::awaitable_t<types::response_t> handle(
        const types::request_t& request,

        next_t next
    ) override {
        if (request.header("X-Short-Circuit").has_value())
            co_return types::text_response_t("short-circuited", types::status_t::ok);

        co_return co_await next(request);
    }
};

class call_tracker_middleware : public c_base_middleware {
  public:
    call_tracker_middleware(int& counter) : counter_(counter) {}

    shared::awaitable_t<types::response_t> handle(
        const types::request_t& request,

        next_t next
    ) override {
        counter_++;

        co_return co_await next(request);
    }

  private:
    int& counter_;
};

class middleware_tests : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;

    shared::awaitable_t<types::response_t> final_handler(const types::request_t& request) {
        co_return types::text_response_t("final handler response");
    }

    next_t compose(
        const std::vector<std::shared_ptr<c_base_middleware>>& middlewares,

        next_t final_h
    ) {
        auto chain = final_h;

        for (auto it = middlewares.rbegin(); it != middlewares.rend(); ++it) {
            auto& mw = *it;

            chain = [mw, next_link = std::move(chain)](const types::request_t& req
                    ) -> shared::awaitable_t<types::response_t> {
                co_return co_await mw->handle(req, next_link);
            };
        }

        return chain;
    }

    void run_awaitable(std::function<boost::asio::awaitable<void>()> test_func) {
        boost::asio::co_spawn(io_context, std::move(test_func), boost::asio::detached);

        io_context.run();

        io_context.restart();
    }
};

TEST_F(middleware_tests, no_middleware) {
    run_awaitable([this]() -> boost::asio::awaitable<void> {
        std::vector<std::shared_ptr<c_base_middleware>> middlewares;

        auto chain = compose(middlewares, [this](const types::request_t& req) { return final_handler(req); });

        types::request_t req;

        auto response = co_await chain(req);

        auto& text_res = dynamic_cast<types::text_response_t&>(response);

        EXPECT_EQ(text_res.body(), "final handler response");

        EXPECT_TRUE(text_res.headers().empty());
    });
}

TEST_F(middleware_tests, single_middleware_modifies_response) {
    run_awaitable([this]() -> boost::asio::awaitable<void> {
        std::vector<std::shared_ptr<c_base_middleware>> middlewares
            = {std::make_shared<add_header_middleware>("X-Header-A", "Value-A")};

        auto chain = compose(middlewares, [this](const types::request_t& req) { return final_handler(req); });

        types::request_t req;

        auto response = co_await chain(req);

        auto& text_res = dynamic_cast<types::text_response_t&>(response);

        EXPECT_EQ(text_res.body(), "final handler response");

        EXPECT_EQ(response.headers().size(), 1);

        EXPECT_EQ(response.headers().at("X-Header-A"), "Value-A");
    });
}

TEST_F(middleware_tests, multiple_middlewares_in_order) {
    run_awaitable([this]() -> boost::asio::awaitable<void> {
        int counter_a = 0;
        int counter_b = 0;

        std::vector<std::shared_ptr<c_base_middleware>> middlewares
            = {std::make_shared<call_tracker_middleware>(counter_a),

               std::make_shared<add_header_middleware>("X-Header-B", "Value-B"),

               std::make_shared<call_tracker_middleware>(counter_b),

               std::make_shared<add_header_middleware>("X-Header-C", "Value-C")};

        auto chain = compose(middlewares, [this](const types::request_t& req) { return final_handler(req); });

        types::request_t req;

        auto response = co_await chain(req);

        auto& text_res = dynamic_cast<types::text_response_t&>(response);

        EXPECT_EQ(text_res.body(), "final handler response");

        EXPECT_EQ(response.headers().size(), 2);

        EXPECT_EQ(response.headers().at("X-Header-B"), "Value-B");
        EXPECT_EQ(response.headers().at("X-Header-C"), "Value-C");

        EXPECT_EQ(counter_a, 1);
        EXPECT_EQ(counter_b, 1);
    });
}

TEST_F(middleware_tests, middleware_short_circuits_chain) {
    run_awaitable([this]() -> boost::asio::awaitable<void> {
        int tracker_called_count = 0;

        std::vector<std::shared_ptr<c_base_middleware>> middlewares
            = {std::make_shared<add_header_middleware>("X-Should-Be-Set", "Value"),

               std::make_shared<short_circuit_middleware>(),

               std::make_shared<call_tracker_middleware>(tracker_called_count)};

        auto chain = compose(middlewares, [this](const types::request_t& req) { return final_handler(req); });

        types::request_t req;

        req.headers()["X-Short-Circuit"] = "true";

        auto response = co_await chain(req);

        auto& text_res = dynamic_cast<types::text_response_t&>(response);

        EXPECT_EQ(text_res.body(), "short-circuited");

        EXPECT_EQ(tracker_called_count, 0);

        EXPECT_EQ(response.headers().size(), 1);

        EXPECT_EQ(response.headers().at("X-Should-Be-Set"), "Value");
    });
}

TEST_F(middleware_tests, middleware_chain_proceeds_when_not_short_circuiting) {
    run_awaitable([this]() -> boost::asio::awaitable<void> {
        int tracker_called_count = 0;

        std::vector<std::shared_ptr<c_base_middleware>> middlewares
            = {std::make_shared<add_header_middleware>("X-Header-A", "Value-A"),

               std::make_shared<short_circuit_middleware>(),

               std::make_shared<call_tracker_middleware>(tracker_called_count)};

        auto chain = compose(middlewares, [this](const types::request_t& req) { return final_handler(req); });

        types::request_t req;

        auto response = co_await chain(req);

        auto& text_res = dynamic_cast<types::text_response_t&>(response);

        EXPECT_EQ(text_res.body(), "final handler response");

        EXPECT_EQ(tracker_called_count, 1);

        EXPECT_EQ(response.headers().size(), 1);

        EXPECT_EQ(response.headers().at("X-Header-A"), "Value-A");
    });
}