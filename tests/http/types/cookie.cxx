#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "clueapi/exceptions/wrap/wrap.hxx"
#include "clueapi/http/types/cookie/cookie.hxx"

class cookie_tests : public ::testing::Test {};

TEST_F(cookie_tests, basic_serialization) {
    using clueapi::http::types::cookie_t;

    cookie_t c{
        "session",
        "12345",
    };

    auto serialized_cookie = cookie_t::serialize(c);

    EXPECT_TRUE(serialized_cookie.has_value());

    auto serialized = serialized_cookie.value();

    EXPECT_NE(serialized.find("session=12345; Path=/; Max-Age=86400;"), std::string::npos);
    EXPECT_NE(serialized.find("Expires="), std::string::npos);
}

TEST_F(cookie_tests, full_serialization) {
    using clueapi::http::types::cookie_t;

    cookie_t c{
        "user",
        "john",
    };

    c.domain()    = "example.com";
    c.path()      = "/profile";
    c.max_age()   = std::chrono::seconds{3600};
    c.secure()    = true;
    c.http_only() = true;
    c.same_site() = "Strict";

    auto serialized_cookie = cookie_t::serialize(c);

    EXPECT_TRUE(serialized_cookie.has_value());

    auto serialized = serialized_cookie.value();

    EXPECT_NE(serialized.find("user=john"), std::string::npos);
    EXPECT_NE(serialized.find("; Domain=example.com"), std::string::npos);
    EXPECT_NE(serialized.find("; Path=/profile"), std::string::npos);
    EXPECT_NE(serialized.find("; Max-Age=3600"), std::string::npos);
    EXPECT_NE(serialized.find("; Expires="), std::string::npos);
    EXPECT_NE(serialized.find("; Secure"), std::string::npos);
    EXPECT_NE(serialized.find("; HttpOnly"), std::string::npos);
    EXPECT_NE(serialized.find("; SameSite=Strict"), std::string::npos);
}

TEST_F(cookie_tests, secure_prefix_validation) {
    using clueapi::http::types::cookie_t;

    using namespace clueapi::exceptions;

    cookie_t c{
        "__Secure-id",
        "abc",
    };

    c.secure() = false;

    auto serialized_cookie = cookie_t::serialize(c);

    EXPECT_FALSE(serialized_cookie.has_value());

    c.secure() = true;

    serialized_cookie = cookie_t::serialize(c);

    EXPECT_TRUE(serialized_cookie.has_value());
}

TEST_F(cookie_tests, host_prefix_validation) {
    using clueapi::http::types::cookie_t;

    using namespace clueapi::exceptions;

    cookie_t c{
        "__Host-id",
        "xyz",
    };

    c.secure() = true;
    c.path()   = "/";

    c.domain()             = "example.com";
    auto serialized_cookie = cookie_t::serialize(c);

    EXPECT_FALSE(serialized_cookie.has_value());
    c.domain() = "";

    c.path()          = "/test";
    serialized_cookie = cookie_t::serialize(c);
    EXPECT_FALSE(serialized_cookie.has_value());
    c.path() = "/";

    c.secure()        = false;
    serialized_cookie = cookie_t::serialize(c);

    EXPECT_FALSE(serialized_cookie.has_value());
    c.secure() = true;

    serialized_cookie = cookie_t::serialize(c);

    EXPECT_TRUE(serialized_cookie.has_value());

    auto serialized = serialized_cookie.value();

    EXPECT_EQ(serialized.find("Domain="), std::string::npos);
}