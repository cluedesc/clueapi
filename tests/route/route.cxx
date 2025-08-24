#include <gtest/gtest.h>

#include <clueapi.hxx>

using namespace clueapi;
using namespace clueapi::http;
using namespace clueapi::route;
using namespace clueapi::route::detail;

class radix_tree_helpers_test : public ::testing::Test {};

TEST_F(radix_tree_helpers_test, norm_path) {
    using tree_t = radix_tree_t<std::shared_ptr<base_route_t>>;

    EXPECT_EQ(tree_t::norm_path(""), "/");
    EXPECT_EQ(tree_t::norm_path("/"), "/");

    EXPECT_EQ(tree_t::norm_path("/users"), "/users");
    EXPECT_EQ(tree_t::norm_path("/users/"), "/users");
    EXPECT_EQ(tree_t::norm_path("/a/b/c/"), "/a/b/c");
}

TEST_F(radix_tree_helpers_test, split_path_segments) {
    using tree_t = radix_tree_t<std::shared_ptr<base_route_t>>;

    EXPECT_TRUE(tree_t::split_path_segments("/").empty());

    EXPECT_EQ(tree_t::split_path_segments("/users"), std::vector<std::string_view>{"users"});
    EXPECT_EQ(
        tree_t::split_path_segments("/users/profile/settings"),
        (std::vector<std::string_view>{"users", "profile", "settings"})
    );
    EXPECT_EQ(tree_t::split_path_segments("/api/v1/"), (std::vector<std::string_view>{"api", "v1"}));
}

TEST_F(radix_tree_helpers_test, is_dynamic_segment) {
    using tree_t = radix_tree_t<std::shared_ptr<base_route_t>>;

    EXPECT_TRUE(tree_t::is_dynamic_segment("{id}"));
    EXPECT_TRUE(tree_t::is_dynamic_segment("{user_id}"));

    EXPECT_FALSE(tree_t::is_dynamic_segment("id"));
    EXPECT_FALSE(tree_t::is_dynamic_segment("{id"));
    EXPECT_FALSE(tree_t::is_dynamic_segment("id}"));
    EXPECT_FALSE(tree_t::is_dynamic_segment("{}"));
}

TEST_F(radix_tree_helpers_test, extract_param_name) {
    using tree_t = radix_tree_t<std::shared_ptr<base_route_t>>;

    EXPECT_EQ(tree_t::extract_param_name("{id}"), "id");
    EXPECT_EQ(tree_t::extract_param_name("{user_id}"), "user_id");
    EXPECT_EQ(tree_t::extract_param_name("{}"), "");
    EXPECT_EQ(tree_t::extract_param_name("id"), "");
}

TEST_F(radix_tree_helpers_test, is_broken_segment) {
    using tree_t = radix_tree_t<std::shared_ptr<base_route_t>>;

    EXPECT_TRUE(tree_t::is_broken_segment("{id"));
    EXPECT_TRUE(tree_t::is_broken_segment("id}"));
    EXPECT_TRUE(tree_t::is_broken_segment("{"));
    EXPECT_TRUE(tree_t::is_broken_segment("}"));

    EXPECT_FALSE(tree_t::is_broken_segment("{id}"));
    EXPECT_FALSE(tree_t::is_broken_segment("id"));
}

class route_handler_test : public ::testing::Test {
  protected:
    boost::asio::io_context io_context;
};

TEST_F(route_handler_test, sync_route) {
    auto sync_handler = [](http::ctx_t ctx) -> types::response_t {
        return types::text_response_t("sync response");
    };

    route_t sync_route(types::method_t::get, "/sync", std::move(sync_handler));

    EXPECT_FALSE(sync_route.is_awaitable());

    auto response = sync_route.handle({});

    EXPECT_EQ(response.body(), "sync response");

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto awaitable_response = co_await sync_route.handle_awaitable({});

            EXPECT_EQ(awaitable_response.body(), "sync response");
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(route_handler_test, async_route) {
    auto async_handler = [](http::ctx_t ctx) -> shared::awaitable_t<http::types::response_t> {
        co_return types::text_response_t("async response");
    };

    route_t async_route(types::method_t::get, "/async", std::move(async_handler));

    EXPECT_TRUE(async_route.is_awaitable());

    EXPECT_THROW(async_route.handle({}), exceptions::exception_t);

    io_context.restart();

    boost::asio::co_spawn(
        io_context,

        [&]() -> boost::asio::awaitable<void> {
            auto response = co_await async_route.handle_awaitable({});

            EXPECT_EQ(response.body(), "async response");
        },

        boost::asio::detached
    );

    io_context.run();
}

TEST_F(route_handler_test, move_semantics) {
    auto sync_handler = [](http::ctx_t ctx) -> types::base_response_t {
        return types::text_response_t("sync response");
    };

    route_t route1(types::method_t::get, "/path", std::move(sync_handler));

    route_t route2(std::move(route1));

    EXPECT_FALSE(route2.is_awaitable());

    auto response2 = route2.handle({});

    EXPECT_EQ(response2.body(), "sync response");

    route_t<decltype(sync_handler)> route3{};

    route3 = std::move(route2);

    EXPECT_FALSE(route3.is_awaitable());

    auto response3 = route3.handle({});

    EXPECT_EQ(response3.body(), "sync response");
}

class radix_tree_tests : public ::testing::Test {
  protected:
    using handler_t = std::function<types::response_t(ctx_t)>;

    radix_tree_t<handler_t> tree;

    ctx_t make_ctx(types::params_t params = {}) {
        types::request_t req;

        return ctx_t(std::move(req), std::move(params));
    }
};

TEST_F(radix_tree_tests, insert_and_find_root) {
    tree.insert(types::method_t::get, "/", [](ctx_t) { return types::text_response_t("root"); });

    auto result = tree.find(types::method_t::get, "/");

    ASSERT_TRUE(result.has_value());

    auto handler = result->first;

    auto response = handler(make_ctx(std::move(result->second)));

    EXPECT_EQ(response.body(), "root");

    EXPECT_TRUE(result->second.empty());
}

TEST_F(radix_tree_tests, insert_and_find_static_route) {
    tree.insert(types::method_t::get, "/hello/world", [](ctx_t) {
        return types::text_response_t("hello world");
    });

    auto result = tree.find(types::method_t::get, "/hello/world");

    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->first(make_ctx({})).body(), "hello world");

    auto not_found = tree.find(types::method_t::get, "/hello");

    EXPECT_FALSE(not_found.has_value());
}

TEST_F(radix_tree_tests, node_splitting) {
    tree.insert(types::method_t::get, "/team", [](ctx_t) { return types::text_response_t("team"); });
    tree.insert(types::method_t::get, "/teams", [](ctx_t) { return types::text_response_t("teams"); });

    auto res1 = tree.find(types::method_t::get, "/team");

    ASSERT_TRUE(res1.has_value());

    EXPECT_EQ(res1->first(make_ctx({})).body(), "team");

    auto res2 = tree.find(types::method_t::get, "/teams");

    ASSERT_TRUE(res2.has_value());

    EXPECT_EQ(res2->first(make_ctx({})).body(), "teams");
}

TEST_F(radix_tree_tests, InsertAndFindDynamicRoute) {
    tree.insert(types::method_t::get, "/users/{id}", [](ctx_t ctx) {
        return types::text_response_t("user " + ctx.params().at("id"));
    });

    auto result = tree.find(types::method_t::get, "/users/123");

    ASSERT_TRUE(result.has_value());

    auto params = result->second;

    auto response = result->first(make_ctx(std::move(params)));

    EXPECT_EQ(response.body(), "user 123");

    ASSERT_EQ(result->second.size(), 1);

    EXPECT_EQ(result->second["id"], "123");
}

TEST_F(radix_tree_tests, prioritize_static_route) {
    tree.insert(types::method_t::get, "/users/profile", [](ctx_t) {
        return types::text_response_t("profile");
    });
    tree.insert(types::method_t::get, "/users/{id}", [](ctx_t ctx) {
        return types::text_response_t("user " + ctx.params().at("id"));
    });

    auto res_dynamic = tree.find(types::method_t::get, "/users/abc");

    ASSERT_TRUE(res_dynamic.has_value());

    EXPECT_EQ(res_dynamic->first(make_ctx(res_dynamic->second)).body(), "user abc");

    auto res_static = tree.find(types::method_t::get, "/users/profile");

    ASSERT_TRUE(res_static.has_value());

    EXPECT_EQ(res_static->first(make_ctx(res_static->second)).body(), "profile");

    EXPECT_TRUE(res_static->second.empty());
}

TEST_F(radix_tree_tests, different_methods_with_same_path) {
    tree.insert(types::method_t::get, "/resource", [](ctx_t) { return types::text_response_t("GET"); });
    tree.insert(types::method_t::post, "/resource", [](ctx_t) { return types::text_response_t("POST"); });

    auto res_get = tree.find(types::method_t::get, "/resource");

    ASSERT_TRUE(res_get.has_value());

    EXPECT_EQ(res_get->first(make_ctx()).body(), "GET");

    auto res_post = tree.find(types::method_t::post, "/resource");

    ASSERT_TRUE(res_post.has_value());

    EXPECT_EQ(res_post->first(make_ctx()).body(), "POST");

    auto res_put = tree.find(types::method_t::put, "/resource");

    EXPECT_FALSE(res_put.has_value());
}

TEST_F(radix_tree_tests, duplicate_route_throws_exception) {
    tree.insert(types::method_t::get, "/duplicate", [](ctx_t) { return types::text_response_t("first"); });

    EXPECT_THROW(
        tree.insert(
            types::method_t::get, "/duplicate", [](ctx_t) { return types::text_response_t("second"); }
        ),

        exceptions::exception_t
    );
}

TEST_F(radix_tree_tests, multiple_dynamic_parameters) {
    tree.insert(types::method_t::get, "/users/{userId}/posts/{postId}", [](ctx_t ctx) {
        return types::text_response_t("user " + ctx.params().at("userId") + " post " + ctx.params().at("postId"));
    });

    auto result = tree.find(types::method_t::get, "/users/123/posts/abc");

    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->second.size(), 2);
    EXPECT_EQ(result->second["userId"], "123");
    EXPECT_EQ(result->second["postId"], "abc");
    EXPECT_EQ(result->first(make_ctx(result->second)).body(), "user 123 post abc");
}

TEST_F(radix_tree_tests, complex_node_splitting) {
    tree.insert(types::method_t::get, "/content", [](ctx_t) { return types::text_response_t("content"); });
    tree.insert(types::method_t::get, "/contact", [](ctx_t) { return types::text_response_t("contact"); });

    auto res1 = tree.find(types::method_t::get, "/content");

    ASSERT_TRUE(res1.has_value());

    EXPECT_EQ(res1->first(make_ctx()).body(), "content");

    auto res2 = tree.find(types::method_t::get, "/contact");

    ASSERT_TRUE(res2.has_value());

    EXPECT_EQ(res2->first(make_ctx()).body(), "contact");
}

TEST_F(radix_tree_tests, ambiguous_dynamic_routes_throw_exception) {
    tree.insert(types::method_t::get, "/users/{id}", [](ctx_t) { return types::text_response_t("id"); });

    EXPECT_THROW(
        tree.insert(
            types::method_t::get, "/users/{uuid}", [](ctx_t) { return types::text_response_t("uuid"); }
        ),

        exceptions::exception_t
    );
}