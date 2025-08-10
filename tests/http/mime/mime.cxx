#include <gtest/gtest.h>

#include <clueapi.hxx>

class mime_tests : public ::testing::Test {};

TEST_F(mime_tests, get_mime_map) {
    using clueapi::http::mime::mime_t;

    const auto& map = mime_t::mime_map();

    ASSERT_FALSE(map.empty());

    EXPECT_EQ(map.size(), 41);

    EXPECT_EQ(map.at(".json"), "application/json");
    EXPECT_EQ(map.at(".png"), "image/png");
}

TEST_F(mime_tests, get_mime_type_from_string) {
    using clueapi::http::mime::mime_t;

    EXPECT_EQ(mime_t::mime_type(".html"), "text/html");
    EXPECT_EQ(mime_t::mime_type(".JPG"), "image/jpeg");
    EXPECT_EQ(mime_t::mime_type(".tIFf"), "image/tiff");

    EXPECT_EQ(mime_t::mime_type(".dat"), "application/octet-stream");

    EXPECT_EQ(mime_t::mime_type(""), "application/octet-stream");
}

TEST_F(mime_tests, get_mime_type_from_path) {
    using clueapi::http::mime::mime_t;

    EXPECT_EQ(mime_t::mime_type(boost::filesystem::path("/var/www/index.html")), "text/html");
    EXPECT_EQ(mime_t::mime_type(boost::filesystem::path("archive.ZIP")), "application/zip");

    EXPECT_EQ(mime_t::mime_type(boost::filesystem::path("my-archive")), "application/octet-stream");

    EXPECT_EQ(mime_t::mime_type(boost::filesystem::path("")), "application/octet-stream");

    EXPECT_EQ(mime_t::mime_type(boost::filesystem::path(".config")), "application/octet-stream");
}