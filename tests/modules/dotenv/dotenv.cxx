#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "clueapi/modules/dotenv/dotenv.hxx"
#include "clueapi/modules/dotenv/detail/hash/hash.hxx"

class dotenv_tests : public ::testing::Test {
  protected:
    void SetUp() override {
        m_test_filename = "test.env";

        std::ofstream test_env_file(m_test_filename);

        {
            test_env_file << "# Comment" << std::endl;
            test_env_file << "  " << std::endl;
            test_env_file << "APP_NAME=MyClueApp" << std::endl;
            test_env_file << "API_VERSION=1.25" << std::endl;
            test_env_file << "PORT=8080" << std::endl;
            test_env_file << "OFFSET=-50" << std::endl;
            test_env_file << "DEBUG_MODE=true" << std::endl;
            test_env_file << "ENABLE_HTTPS=False" << std::endl;
            test_env_file << "  DB_HOST   =   localhost  " << std::endl;
            test_env_file << "EMPTY_VALUE=" << std::endl;
            test_env_file << "MALFORMED_LINE_NO_EQUALS" << std::endl;
            test_env_file << "SECRET_KEY= a b c " << std::endl;
        }

        test_env_file.close();

        m_dotenv.load(m_test_filename);
    }

    void TearDown() override {
        m_dotenv.destroy();

        std::remove(m_test_filename.c_str());
    }

    std::string m_test_filename;

    clueapi::modules::dotenv::c_dotenv m_dotenv;
};

TEST_F(dotenv_tests, key_existence) {
    EXPECT_TRUE(m_dotenv.contains(ENV_NAME_RT("APP_NAME")));
    EXPECT_TRUE(m_dotenv.contains(ENV_NAME_RT("PORT")));
    EXPECT_TRUE(m_dotenv.contains(ENV_NAME_RT("DB_HOST")));
    EXPECT_TRUE(m_dotenv.contains(ENV_NAME_RT("EMPTY_VALUE")));

    EXPECT_FALSE(m_dotenv.contains(ENV_NAME_RT("NON_EXISTENT_KEY")));
    EXPECT_FALSE(m_dotenv.contains(ENV_NAME_RT("MALFORMED_LINE_NO_EQUALS")));
}

TEST_F(dotenv_tests, string_retrieval) {
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("APP_NAME")), "MyClueApp");
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("DB_HOST")), "   localhost  ");
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("EMPTY_VALUE")), "");
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("SECRET_KEY")), " a b c ");
}

TEST_F(dotenv_tests, integer_retrieval) {
    EXPECT_EQ(m_dotenv.at<int>(ENV_NAME_RT("PORT")), 8080);
    EXPECT_EQ(m_dotenv.at<long>(ENV_NAME_RT("PORT")), 8080L);
    EXPECT_EQ(m_dotenv.at<int>(ENV_NAME_RT("OFFSET")), -50);
}

TEST_F(dotenv_tests, floating_point_retrieval) {
    EXPECT_FLOAT_EQ(m_dotenv.at<float>(ENV_NAME_RT("API_VERSION")), 1.25f);
    EXPECT_DOUBLE_EQ(m_dotenv.at<double>(ENV_NAME_RT("API_VERSION")), 1.25);
}

TEST_F(dotenv_tests, boolean_retrieval) {
    EXPECT_TRUE(m_dotenv.at<bool>(ENV_NAME_RT("DEBUG_MODE")));
    EXPECT_FALSE(m_dotenv.at<bool>(ENV_NAME_RT("ENABLE_HTTPS")));

    EXPECT_FALSE(m_dotenv.at<bool>(ENV_NAME_RT("APP_NAME")));
    EXPECT_FALSE(m_dotenv.at<bool>(ENV_NAME_RT("PORT")));
}

TEST(dotenv_standalone_tests, file_not_found) {
    clueapi::modules::dotenv::c_dotenv dotenv{};

    dotenv.load("non_existent_file.env");

    EXPECT_FALSE(dotenv.contains(ENV_NAME_RT("ANY_KEY")));
}

TEST_F(dotenv_tests, destroy_method) {
    ASSERT_TRUE(m_dotenv.contains(ENV_NAME_RT("APP_NAME")));

    m_dotenv.destroy();

    EXPECT_FALSE(m_dotenv.contains(ENV_NAME_RT("APP_NAME")));
}

TEST_F(dotenv_tests, reinitialization) {
    std::string new_filename = "test2.env";

    std::ofstream new_file(new_filename);

    { new_file << "NEW_KEY=NewValue" << std::endl; }

    new_file.close();

    m_dotenv.load(new_filename);

    EXPECT_TRUE(m_dotenv.contains(ENV_NAME_RT("NEW_KEY")));

    std::remove(new_filename.c_str());
}

TEST_F(dotenv_tests, trim_values_enabled) {
    m_dotenv.load(m_test_filename, true);

    ASSERT_TRUE(m_dotenv.contains(ENV_NAME_RT("DB_HOST")));
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("DB_HOST")), "localhost");

    ASSERT_TRUE(m_dotenv.contains(ENV_NAME_RT("SECRET_KEY")));
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("SECRET_KEY")), "a b c");
}

TEST_F(dotenv_tests, trim_values_disabled_by_default) {
    m_dotenv.load(m_test_filename);

    ASSERT_TRUE(m_dotenv.contains(ENV_NAME_RT("DB_HOST")));
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("DB_HOST")), "   localhost  ");

    ASSERT_TRUE(m_dotenv.contains(ENV_NAME_RT("SECRET_KEY")));
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("SECRET_KEY")), " a b c ");
}

TEST_F(dotenv_tests, DefaultValueRetrieval) {
    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("NON_EXISTENT_KEY"), "default_app"), "default_app");

    EXPECT_EQ(m_dotenv.at<int>(ENV_NAME_RT("NON_EXISTENT_KEY"), 9999), 9999);

    EXPECT_DOUBLE_EQ(m_dotenv.at<double>(ENV_NAME_RT("NON_EXISTENT_KEY"), 3.14), 3.14);

    EXPECT_TRUE(m_dotenv.at<bool>(ENV_NAME_RT("NON_EXISTENT_KEY"), true));
    EXPECT_FALSE(m_dotenv.at<bool>(ENV_NAME_RT("NON_EXISTENT_KEY"), false));

    EXPECT_EQ(m_dotenv.at<std::string>(ENV_NAME_RT("APP_NAME"), "default_app"), "MyClueApp");
    EXPECT_EQ(m_dotenv.at<int>(ENV_NAME_RT("PORT"), 9999), 8080);
}

TEST_F(dotenv_tests, MacroUsage) {
    auto app_name = CLUEAPI_DOTENV_GET_IMPL(m_dotenv, ENV_NAME_RT("APP_NAME"), std::string, "");

    EXPECT_EQ(app_name, "MyClueApp");

    auto port = CLUEAPI_DOTENV_GET_IMPL(m_dotenv, ENV_NAME_RT("PORT"), int, 0);

    EXPECT_EQ(port, 8080);

    auto default_val = CLUEAPI_DOTENV_GET_IMPL(m_dotenv, ENV_NAME_RT("MISSING_KEY"), std::string, "default value");

    EXPECT_EQ(default_val, "default value");

    auto* dotenv_ptr = &m_dotenv;

    auto host_name = CLUEAPI_DOTENV_GET_IMPL(dotenv_ptr, ENV_NAME_RT("DB_HOST"), std::string, "");

    EXPECT_EQ(host_name, "   localhost  ");

    auto default_int = CLUEAPI_DOTENV_GET_IMPL(dotenv_ptr, ENV_NAME_RT("MISSING_INT"), int, -1);

    EXPECT_EQ(default_int, -1);
}