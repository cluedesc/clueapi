#include <gtest/gtest.h>

#include <clueapi.hxx>

class method_tests : public ::testing::Test {};

TEST_F(method_tests, to_str_conversion) {
    using clueapi::http::types::method_t;

    EXPECT_EQ(method_t::to_str(method_t::e_method::unknown), "UNKNOWN");
    EXPECT_EQ(method_t::to_str(method_t::e_method::get), "GET");
    EXPECT_EQ(method_t::to_str(method_t::e_method::head), "HEAD");
    EXPECT_EQ(method_t::to_str(method_t::e_method::post), "POST");
    EXPECT_EQ(method_t::to_str(method_t::e_method::put), "PUT");
    EXPECT_EQ(method_t::to_str(method_t::e_method::delete_), "DELETE");
    EXPECT_EQ(method_t::to_str(method_t::e_method::connect), "CONNECT");
    EXPECT_EQ(method_t::to_str(method_t::e_method::options), "OPTIONS");
    EXPECT_EQ(method_t::to_str(method_t::e_method::trace), "TRACE");
    EXPECT_EQ(method_t::to_str(method_t::e_method::patch), "PATCH");

    EXPECT_EQ(method_t::to_str(static_cast<method_t::e_method>(999)), "UNKNOWN");
}

TEST_F(method_tests, from_str_conversion) {
    using clueapi::http::types::method_t;

    EXPECT_EQ(method_t::from_str("GET"), method_t::e_method::get);
    EXPECT_EQ(method_t::from_str("HEAD"), method_t::e_method::head);
    EXPECT_EQ(method_t::from_str("POST"), method_t::e_method::post);
    EXPECT_EQ(method_t::from_str("PUT"), method_t::e_method::put);
    EXPECT_EQ(method_t::from_str("DELETE"), method_t::e_method::delete_);
    EXPECT_EQ(method_t::from_str("CONNECT"), method_t::e_method::connect);
    EXPECT_EQ(method_t::from_str("OPTIONS"), method_t::e_method::options);
    EXPECT_EQ(method_t::from_str("TRACE"), method_t::e_method::trace);
    EXPECT_EQ(method_t::from_str("PATCH"), method_t::e_method::patch);
}

TEST_F(method_tests, from_str_unknown_and_case) {
    using clueapi::http::types::method_t;

    EXPECT_EQ(method_t::from_str("INVALID"), method_t::e_method::unknown);

    EXPECT_EQ(method_t::from_str(""), method_t::e_method::unknown);

    EXPECT_EQ(method_t::from_str("get"), method_t::e_method::unknown);

    EXPECT_EQ(method_t::from_str("GETT"), method_t::e_method::unknown);
}