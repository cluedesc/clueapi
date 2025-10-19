#include <gtest/gtest.h>

#include "clueapi/http/types/method/method.hxx"

class method_tests : public ::testing::Test {};

TEST_F(method_tests, to_str_conversion) {
    using clueapi::http::types::method_t;

    EXPECT_EQ(method_t::to_str(method_t::unknown), "UNKNOWN");
    EXPECT_EQ(method_t::to_str(method_t::get), "GET");
    EXPECT_EQ(method_t::to_str(method_t::head), "HEAD");
    EXPECT_EQ(method_t::to_str(method_t::post), "POST");
    EXPECT_EQ(method_t::to_str(method_t::put), "PUT");
    EXPECT_EQ(method_t::to_str(method_t::delete_), "DELETE");
    EXPECT_EQ(method_t::to_str(method_t::connect), "CONNECT");
    EXPECT_EQ(method_t::to_str(method_t::options), "OPTIONS");
    EXPECT_EQ(method_t::to_str(method_t::trace), "TRACE");
    EXPECT_EQ(method_t::to_str(method_t::patch), "PATCH");

    EXPECT_EQ(method_t::to_str(static_cast<method_t::e_method>(999)), "UNKNOWN");
}

TEST_F(method_tests, from_str_conversion) {
    using clueapi::http::types::method_t;

    EXPECT_EQ(method_t::from_str("GET"), method_t::get);
    EXPECT_EQ(method_t::from_str("HEAD"), method_t::head);
    EXPECT_EQ(method_t::from_str("POST"), method_t::post);
    EXPECT_EQ(method_t::from_str("PUT"), method_t::put);
    EXPECT_EQ(method_t::from_str("DELETE"), method_t::delete_);
    EXPECT_EQ(method_t::from_str("CONNECT"), method_t::connect);
    EXPECT_EQ(method_t::from_str("OPTIONS"), method_t::options);
    EXPECT_EQ(method_t::from_str("TRACE"), method_t::trace);
    EXPECT_EQ(method_t::from_str("PATCH"), method_t::patch);
}

TEST_F(method_tests, from_str_unknown_and_case) {
    using clueapi::http::types::method_t;

    EXPECT_EQ(method_t::from_str("INVALID"), method_t::unknown);

    EXPECT_EQ(method_t::from_str(""), method_t::unknown);

    EXPECT_EQ(method_t::from_str("get"), method_t::unknown);

    EXPECT_EQ(method_t::from_str("GETT"), method_t::unknown);
}