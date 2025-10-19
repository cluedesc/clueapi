#include <gtest/gtest.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <functional>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "clueapi/exceptions/wrap/wrap.hxx"
#include "clueapi/http/multipart/multipart.hxx"
#include "clueapi/http/types/file/file.hxx"

using namespace clueapi::http;
using namespace clueapi::http::types;
using namespace clueapi::http::multipart;
using namespace clueapi::exceptions;

std::string create_multipart_body(
    const std::string& boundary,
    const std::map<std::string, std::string>& fields,
    const std::map<std::string, std::pair<std::string, std::string>>& files,
    const std::map<std::string, std::pair<std::string, std::string>>& files_utf8 = {}) {
    std::stringstream body;

    const std::string crlf = "\r\n";

    for (const auto& field : fields) {
        body << "--" << boundary << crlf;
        body << "Content-Disposition: form-data; name=\"" << field.first << "\"" << crlf;

        body << crlf;
        body << field.second << crlf;
    }

    for (const auto& file : files) {
        body << "--" << boundary << crlf;

        body << "Content-Disposition: form-data; name=\"" << file.first << "\"; filename=\""
             << file.second.first << "\"" << crlf;
        body << "Content-Type: application/octet-stream" << crlf;

        body << crlf;
        body << file.second.second << crlf;
    }

    for (const auto& file : files_utf8) {
        body << "--" << boundary << crlf;

        body << "Content-Disposition: form-data; name=\"" << file.first
             << "\"; filename*=" << file.second.first << crlf;
        body << "Content-Type: application/octet-stream" << crlf;

        body << crlf;
        body << file.second.second << crlf;
    }

    body << "--" << boundary << "--" << crlf;

    return body.str();
}

class multipart_parser_tests : public ::testing::Test {
   protected:
    void SetUp() override {
        temp_dir_ = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

        boost::filesystem::create_directory(temp_dir_);
    }

    void TearDown() override {
        io_context_.stop();

        while (!io_context_.stopped())
            io_context_.run_one();

        boost::filesystem::remove_all(temp_dir_);
    }

    boost::filesystem::path create_temp_file(const std::string& content) {
        auto path = temp_dir_ / boost::filesystem::unique_path();

        std::ofstream file(path.string(), std::ios::binary);

        file << content;

        file.close();

        return path;
    }

    void run_test(std::function<boost::asio::awaitable<void>()> awaitable) {
        boost::asio::co_spawn(io_context_, std::move(awaitable), boost::asio::detached);

        io_context_.run();
    }

    boost::asio::io_context io_context_;

    boost::filesystem::path temp_dir_;

    const std::string boundary_ = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
};

TEST_F(multipart_parser_tests, parse_single_in_memory_file) {
    run_test([this]() -> boost::asio::awaitable<void> {
        const std::string file_content = "simple file content";

        auto body = create_multipart_body(
            boundary_, {{"field1", "value1"}}, {{"file1", {"test.txt", file_content}}});

        parser_t::cfg_t cfg{
            .m_boundary = boundary_,
            .m_max_file_size_in_memory = 1024,
            .m_max_files_size_in_memory = 2048};

        parser_t parser(cfg);

        auto result = co_await parser.parse(body);

        auto value = std::move(result).value();

        EXPECT_EQ(value.m_fields.size(), 1);

        auto field_it = value.m_fields.find("field1");

        EXPECT_NE(field_it, value.m_fields.end());

        const auto& field = field_it->second;

        EXPECT_EQ(field, "value1");

        EXPECT_EQ(value.m_files.size(), 1);

        auto file_it = value.m_files.find("file1");

        EXPECT_NE(file_it, value.m_files.end());

        const auto& file = file_it->second;

        EXPECT_EQ(file.name(), "test.txt");

        EXPECT_TRUE(file.in_memory());

        EXPECT_EQ(file.size(), file_content.size());
    });
}

TEST_F(multipart_parser_tests, parse_file_spills_to_disk_on_size_limit) {
    run_test([this]() -> boost::asio::awaitable<void> {
        const std::string large_file_content(200, 'A');

        auto body = create_multipart_body(
            boundary_, {}, {{"largefile", {"large.txt", large_file_content}}});

        parser_t::cfg_t cfg{
            .m_boundary = boundary_,
            .m_max_file_size_in_memory = 100,
            .m_max_files_size_in_memory = 1024};

        parser_t parser(cfg);

        auto result = co_await parser.parse(body);

        EXPECT_TRUE(result.has_value());

        auto value = std::move(result).value();

        EXPECT_EQ(value.m_files.size(), 1);

        auto file_it = value.m_files.find("largefile");

        EXPECT_NE(file_it, value.m_files.end());

        const auto& file = file_it->second;

        EXPECT_EQ(file.name(), "large.txt");

        EXPECT_FALSE(file.in_memory());

        EXPECT_EQ(file.size(), large_file_content.size());

        EXPECT_FALSE(file.temp_path().empty());

        EXPECT_TRUE(boost::filesystem::exists(file.temp_path()));
    });
}

TEST_F(multipart_parser_tests, parse_spills_subsequent_files_on_total_size_limit) {
    run_test([this]() -> boost::asio::awaitable<void> {
        const std::string file_content_1(80, 'A');
        const std::string file_content_2(80, 'B');

        auto body = create_multipart_body(
            boundary_,

            {},

            {{"file1", {"file1.txt", file_content_1}}, {"file2", {"file2.txt", file_content_2}}});

        parser_t::cfg_t cfg{
            .m_boundary = boundary_,
            .m_max_file_size_in_memory = 100,
            .m_max_files_size_in_memory = 150};

        parser_t parser(cfg);

        auto result = co_await parser.parse(body);

        EXPECT_TRUE(result.has_value());

        auto value = std::move(result).value();

        EXPECT_EQ(value.m_files.size(), 2);

        auto file1_it = value.m_files.find("file1");

        EXPECT_NE(file1_it, value.m_files.end());

        EXPECT_TRUE(file1_it->second.in_memory());

        auto file2_it = value.m_files.find("file2");

        EXPECT_NE(file2_it, value.m_files.end());

        EXPECT_FALSE(file2_it->second.in_memory());
    });
}

TEST_F(multipart_parser_tests, parse_utf8_filename) {
    run_test([this]() -> boost::asio::awaitable<void> {
        // RFC specifies this format for non-ASCII filenames
        const std::string utf8_filename_encoded = "UTF-8''%D0%BF%D1%80%D0%B8%D0%B2%D0%B5%D1%82.txt";
        const std::string utf8_filename_decoded = "привет.txt";

        auto body = create_multipart_body(
            boundary_, {}, {}, {{"utf8file", {utf8_filename_encoded, "content"}}});

        parser_t::cfg_t cfg{.m_boundary = boundary_};

        parser_t parser(cfg);

        auto result = co_await parser.parse(body);

        EXPECT_TRUE(result.has_value());

        auto value = std::move(result).value();

        EXPECT_EQ(value.m_files.size(), 1);

        auto file_it = value.m_files.find("utf8file");

        EXPECT_NE(file_it, value.m_files.end());

        EXPECT_EQ(file_it->second.name(), utf8_filename_decoded);
    });
}

TEST_F(multipart_parser_tests, parse_fails_on_malformed_body_no_final_boundary) {
    run_test([this]() -> boost::asio::awaitable<void> {
        auto body = create_multipart_body(boundary_, {{"field", "value"}}, {});

        body = body.substr(0, body.find(boundary_ + "--"));

        parser_t::cfg_t cfg{.m_boundary = boundary_};

        parser_t parser(cfg);

        auto result = co_await parser.parse(body);

        EXPECT_FALSE(result.has_value());

        EXPECT_NE(result.error().find("Can't find content end section"), std::string::npos);
    });
}

TEST_F(multipart_parser_tests, parse_file_successful) {
    run_test([this]() -> boost::asio::awaitable<void> {
        const std::string file_content = "file content from disk";

        auto body = create_multipart_body(
            boundary_,

            {{"field_disk", "value_disk"}},

            {{"file_disk", {"test_disk.txt", file_content}}});

        auto temp_file_path = create_temp_file(body);

        parser_t::cfg_t cfg{.m_boundary = boundary_};

        parser_t parser(cfg);

        auto result = co_await parser.parse_file(temp_file_path);

        EXPECT_TRUE(result.has_value());

        if (!result.has_value())
            std::cout << "Parsing failed: " << result.error() << "\n";

        auto value = std::move(result).value();

        EXPECT_EQ(value.m_fields.size(), 1);

        auto field_it = value.m_fields.find("field_disk");

        EXPECT_NE(field_it, value.m_fields.end());

        EXPECT_EQ(field_it->second, "value_disk");

        EXPECT_EQ(value.m_files.size(), 1);

        auto file_it = value.m_files.find("file_disk");

        EXPECT_NE(file_it, value.m_files.end());

        EXPECT_EQ(file_it->second.name(), "test_disk.txt");

        EXPECT_TRUE(file_it->second.in_memory());

        EXPECT_EQ(file_it->second.size(), file_content.size());
    });
}

TEST_F(multipart_parser_tests, parse_file_with_split_boundary) {
    run_test([this]() -> boost::asio::awaitable<void> {
        const std::size_t chunk_size = 60;

        const std::string part1_content(80, 'A');
        const std::string part2_content = "part2";

        std::stringstream body_ss;

        const std::string crlf = "\r\n";
        const std::string dash_boundary = "--" + boundary_;
        const std::string crlf_dash_boundary = crlf + dash_boundary;

        body_ss << dash_boundary << crlf;
        body_ss << "Content-Disposition: form-data; name=\"part1\"" << crlf << crlf;
        body_ss << part1_content;

        body_ss << crlf_dash_boundary << crlf;

        body_ss << "Content-Disposition: form-data; name=\"part2\"" << crlf << crlf;
        body_ss << part2_content << crlf;
        body_ss << dash_boundary << "--" << crlf;

        auto body = body_ss.str();
        auto temp_file_path = create_temp_file(body);

        parser_t::cfg_t cfg{.m_boundary = boundary_, .m_chunk_size = chunk_size};

        parser_t parser(cfg);

        auto result = co_await parser.parse_file(temp_file_path);

        EXPECT_TRUE(result.has_value()) << result.error();

        auto value = std::move(result).value();

        EXPECT_EQ(value.m_fields.size(), 2);

        auto part1_it = value.m_fields.find("part1");

        EXPECT_NE(part1_it, value.m_fields.end());

        EXPECT_EQ(part1_it->second, part1_content);

        auto part2_it = value.m_fields.find("part2");

        EXPECT_NE(part2_it, value.m_fields.end());

        EXPECT_EQ(part2_it->second, part2_content);
    });
}

TEST_F(multipart_parser_tests, parse_file_fails_on_non_existent_file) {
    run_test([this]() -> boost::asio::awaitable<void> {
        auto non_existent_path = temp_dir_ / "i_do_not_exist.txt";

        parser_t::cfg_t cfg{.m_boundary = boundary_};

        parser_t parser(cfg);

        auto result = co_await parser.parse_file(non_existent_path);

        EXPECT_FALSE(result.has_value());

        EXPECT_NE(result.error().find("Failed to open file"), std::string::npos);
    });
}

TEST_F(multipart_parser_tests, parse_file_fails_on_unexpected_eof) {
    run_test([this]() -> boost::asio::awaitable<void> {
        std::string body = "--" + boundary_ + "\r\n" +
                           "Content-Disposition: form-data; name=\"field\"\r\n\r\n" + "value\r\n";

        auto temp_file_path = create_temp_file(body);

        parser_t::cfg_t cfg{.m_boundary = boundary_};

        parser_t parser(cfg);

        auto result = co_await parser.parse_file(temp_file_path);

        EXPECT_FALSE(result.has_value());

        EXPECT_NE(result.error().find("Field boundary not found. EOF reached"), std::string::npos);
    });
}