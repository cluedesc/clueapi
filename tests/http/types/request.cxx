#include <gtest/gtest.h>

#include <optional>

#include "clueapi/http/types/method/method.hxx"
#include "clueapi/http/types/request/request.hxx"

class request_tests : public ::testing::Test {
  protected:
    clueapi::http::types::request_t req;
};

TEST_F(request_tests, initial_state) {
    EXPECT_EQ(req.method(), clueapi::http::types::method_t::unknown);

    EXPECT_TRUE(req.uri().empty());
    EXPECT_TRUE(req.body().empty());
    EXPECT_TRUE(req.headers().empty());
    EXPECT_TRUE(req.parse_path().empty());

    EXPECT_TRUE(req.keep_alive());
}

TEST_F(request_tests, header_retrieval) {
    req.headers()["Content-Type"] = "application/json";
    req.headers()["Host"]         = "example.com";

    auto content_type = req.header("content-type");

    ASSERT_TRUE(content_type.has_value());
    EXPECT_EQ(content_type.value(), "application/json");

    auto host = req.header("Host");

    ASSERT_TRUE(host.has_value());
    EXPECT_EQ(host.value(), "example.com");

    auto non_existent = req.header("X-Non-Existent");

    EXPECT_FALSE(non_existent.has_value());
}

TEST_F(request_tests, keep_alive_logic) {
    req.headers()["Connection"] = "keep-alive";

    EXPECT_TRUE(req.keep_alive());

    req.headers()["Connection"] = "Keep-Alive";

    EXPECT_TRUE(req.keep_alive());

    req.headers()["Connection"] = "close";

    EXPECT_FALSE(req.keep_alive());

    req.headers().clear();

    EXPECT_TRUE(req.keep_alive());
}

TEST_F(request_tests, cookie_retrieval_simple) {
    req.headers()["cookie"] = "session_id=abc123";

    auto cookie = req.cookie("session_id");

    ASSERT_TRUE(cookie.has_value());
    EXPECT_EQ(cookie.value(), "abc123");

    auto non_existent = req.cookie("user_id");

    EXPECT_FALSE(non_existent.has_value());
}

TEST_F(request_tests, cookie_retrieval_multiple) {
    req.headers()["Cookie"] = "user=johndoe; theme=dark; tracking=off";

    auto user = req.cookie("user");

    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user.value(), "johndoe");

    auto theme = req.cookie("theme");

    ASSERT_TRUE(theme.has_value());
    EXPECT_EQ(theme.value(), "dark");

    auto tracking = req.cookie("tracking");

    ASSERT_TRUE(tracking.has_value());
    EXPECT_EQ(tracking.value(), "off");
}

TEST_F(request_tests, cookie_retrieval_with_whitespace) {
    req.headers()["cookie"] = " key1 = value1 ;  key2=value2  ; key3 = value3 ";

    auto key1 = req.cookie("key1");

    ASSERT_TRUE(key1.has_value());
    EXPECT_EQ(key1.value(), "value1");

    auto key2 = req.cookie("key2");

    ASSERT_TRUE(key2.has_value());
    EXPECT_EQ(key2.value(), "value2");

    auto key3 = req.cookie("key3");

    ASSERT_TRUE(key3.has_value());
    EXPECT_EQ(key3.value(), "value3");
}

TEST_F(request_tests, cookie_no_header) { EXPECT_FALSE(req.cookie("any").has_value()); }

TEST_F(request_tests, cookie_empty_header) {
    req.headers()["cookie"] = "";

    EXPECT_FALSE(req.cookie("any").has_value());
}

TEST_F(request_tests, cookie_malformed_and_valid) {
    req.headers()["cookie"] = "malformed; key=value";

    auto malformed = req.cookie("malformed");

    EXPECT_FALSE(malformed.has_value());

    auto key = req.cookie("key");

    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(key.value(), "value");
}

TEST_F(request_tests, cookie_with_empty_value) {
    req.headers()["cookie"] = "empty_val=; key2=val2";

    auto empty_val = req.cookie("empty_val");

    ASSERT_TRUE(empty_val.has_value());
    EXPECT_EQ(empty_val.value(), "");
}

TEST_F(request_tests, get_all_cookies) {
    req.headers()["cookie"] = "id=123; pref=high";

    const auto& all_cookies = req.cookies();

    EXPECT_EQ(all_cookies.size(), 2);

    EXPECT_EQ(all_cookies.at("id"), "123");
    EXPECT_EQ(all_cookies.at("pref"), "high");

    const auto& all_cookies_again = req.cookies();

    EXPECT_EQ(all_cookies_again.size(), 2);
}

TEST_F(request_tests, accessors) {
    req.method() = clueapi::http::types::method_t::post;

    EXPECT_EQ(req.method(), clueapi::http::types::method_t::post);

    req.uri() = clueapi::http::types::uri_t{"/test/path"};

    EXPECT_EQ(req.uri(), "/test/path");

    req.body() = "data";

    EXPECT_EQ(req.body(), "data");

    req.parse_path() = "/usr/local/bin";

    EXPECT_EQ(req.parse_path(), "/usr/local/bin");
}