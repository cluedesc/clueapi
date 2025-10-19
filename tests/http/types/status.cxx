#include <gtest/gtest.h>

#include "clueapi/http/types/status/status.hxx"

class status_tests : public ::testing::Test {};

TEST_F(status_tests, to_str_conversion) {
    using clueapi::http::types::status_t;

    // 1xx Informational
    EXPECT_EQ(status_t::to_str(status_t::continue_status), "Continue");
    EXPECT_EQ(status_t::to_str(status_t::switching_protocols), "Switching Protocols");

    // 2xx Success
    EXPECT_EQ(status_t::to_str(status_t::ok), "OK");
    EXPECT_EQ(status_t::to_str(status_t::created), "Created");
    EXPECT_EQ(status_t::to_str(status_t::no_content), "No Content");

    // 3xx Redirection
    EXPECT_EQ(status_t::to_str(status_t::moved_permanently), "Moved Permanently");
    EXPECT_EQ(status_t::to_str(status_t::not_modified), "Not Modified");

    // 4xx Client Error
    EXPECT_EQ(status_t::to_str(status_t::bad_request), "Bad Request");
    EXPECT_EQ(status_t::to_str(status_t::unauthorized), "Unauthorized");
    EXPECT_EQ(status_t::to_str(status_t::not_found), "Not Found");
    EXPECT_EQ(status_t::to_str(status_t::im_a_teapot), "I'm a teapot");

    // 5xx Server Error
    EXPECT_EQ(status_t::to_str(status_t::internal_server_error), "Internal Server Error");
    EXPECT_EQ(status_t::to_str(status_t::not_implemented), "Not Implemented");
    EXPECT_EQ(status_t::to_str(status_t::service_unavailable), "Service Unavailable");

    // Unknown and default cases
    EXPECT_EQ(status_t::to_str(status_t::unknown), "Unknown");
    EXPECT_EQ(status_t::to_str(static_cast<status_t::e_status>(999)), "Unknown");
}