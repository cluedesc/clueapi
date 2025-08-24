#include <gtest/gtest.h>

#include <clueapi.hxx>

class file_tests : public ::testing::Test {
  protected:
    boost::filesystem::path create_temp_file(const std::string& content) {
        auto temp_path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

        std::ofstream temp_file(temp_path.string(), std::ios::binary);

        temp_file << content;
        temp_file.close();

        return temp_path;
    }
};

TEST_F(file_tests, in_memory_file) {
    using clueapi::http::types::file_t;

    std::vector<char> data = {'t', 'e', 's', 't'};

    file_t file("report.txt", "text/plain", data);

    EXPECT_EQ(file.name(), "report.txt");
    EXPECT_EQ(file.content_type(), "text/plain");

    EXPECT_EQ(file.data(), data);

    EXPECT_TRUE(file.in_memory());

    EXPECT_EQ(file.size(), 4);

    EXPECT_TRUE(file.temp_path().empty());
}

TEST_F(file_tests, on_disk_file_destructor) {
    using clueapi::http::types::file_t;

    auto temp_path = create_temp_file("file content");

    ASSERT_TRUE(boost::filesystem::exists(temp_path));

    {
        file_t file("temp.dat", "application/octet-stream", temp_path);

        EXPECT_FALSE(file.in_memory());

        EXPECT_EQ(file.size(), 12);

        EXPECT_EQ(file.temp_path(), temp_path);
    }

    EXPECT_FALSE(boost::filesystem::exists(temp_path));
}

TEST_F(file_tests, move_constructor) {
    using clueapi::http::types::file_t;

    auto temp_path = create_temp_file("move test");

    ASSERT_TRUE(boost::filesystem::exists(temp_path));

    file_t file1("temp.dat", "application/octet-stream", temp_path);

    file_t file2(std::move(file1));

    EXPECT_EQ(file2.temp_path(), temp_path);
    EXPECT_FALSE(file2.in_memory());

    EXPECT_TRUE(file1.temp_path().empty());
}

TEST_F(file_tests, move_assignment) {
    using clueapi::http::types::file_t;

    auto temp_path1 = create_temp_file("source_content");
    auto temp_path2 = create_temp_file("target_content");

    ASSERT_TRUE(boost::filesystem::exists(temp_path1));
    ASSERT_TRUE(boost::filesystem::exists(temp_path2));

    file_t file1("source.txt", "text/plain", temp_path1);
    file_t file2("target.bin", "application/octet-stream", temp_path2);

    file2 = std::move(file1);

    EXPECT_FALSE(boost::filesystem::exists(temp_path2));

    EXPECT_EQ(file2.name(), "source.txt");
    EXPECT_EQ(file2.temp_path(), temp_path1);

    EXPECT_TRUE(file1.temp_path().empty());

    EXPECT_TRUE(boost::filesystem::exists(temp_path1));
}
