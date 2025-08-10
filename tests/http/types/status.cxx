#include <gtest/gtest.h>

#include <clueapi.hxx>

class status_tests : public ::testing::Test {};

TEST_F(status_tests, to_str_conversion) {
    using clueapi::http::types::status_t;

    // 1xx Informational
    EXPECT_EQ(status_t::to_str(status_t::e_status::continue_status), "Continue");
    EXPECT_EQ(status_t::to_str(status_t::e_status::switching_protocols), "Switching Protocols");

    // 2xx Success
    EXPECT_EQ(status_t::to_str(status_t::e_status::ok), "OK");
    EXPECT_EQ(status_t::to_str(status_t::e_status::created), "Created");
    EXPECT_EQ(status_t::to_str(status_t::e_status::no_content), "No Content");

    // 3xx Redirection
    EXPECT_EQ(status_t::to_str(status_t::e_status::moved_permanently), "Moved Permanently");
    EXPECT_EQ(status_t::to_str(status_t::e_status::not_modified), "Not Modified");

    // 4xx Client Error
    EXPECT_EQ(status_t::to_str(status_t::e_status::bad_request), "Bad Request");
    EXPECT_EQ(status_t::to_str(status_t::e_status::unauthorized), "Unauthorized");
    EXPECT_EQ(status_t::to_str(status_t::e_status::not_found), "Not Found");
    EXPECT_EQ(status_t::to_str(status_t::e_status::im_a_teapot), "I'm a teapot");

    // 5xx Server Error
    EXPECT_EQ(status_t::to_str(status_t::e_status::internal_server_error), "Internal Server Error");
    EXPECT_EQ(status_t::to_str(status_t::e_status::not_implemented), "Not Implemented");
    EXPECT_EQ(status_t::to_str(status_t::e_status::service_unavailable), "Service Unavailable");

    // Unknown and default cases
    EXPECT_EQ(status_t::to_str(status_t::e_status::unknown), "Unknown");
    EXPECT_EQ(status_t::to_str(static_cast<status_t::e_status>(999)), "Unknown");
}