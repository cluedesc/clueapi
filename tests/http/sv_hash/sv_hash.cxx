#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <unordered_map>

#include "clueapi/http/detail/sv_hash/sv_hash.hxx"

using clueapi::http::detail::sv_eq_t;
using clueapi::http::detail::sv_hash_t;

class sv_hash_tests : public ::testing::Test {};

TEST_F(sv_hash_tests, sv_hash_produces_consistent_hashes) {
    sv_hash_t hasher;

    const std::string_view sv1 = "hello world";
    const std::string_view sv2 = "hello world";
    const std::string_view sv3 = "hello there";

    EXPECT_EQ(hasher(sv1), hasher(sv2));

    EXPECT_NE(hasher(sv1), hasher(sv3));

    const std::string_view empty_sv = "";

    EXPECT_NO_THROW(hasher(empty_sv));
}

TEST_F(sv_hash_tests, sv_eq_compares_correctly) {
    sv_eq_t equality_checker;

    const std::string_view sv1 = "test";
    const std::string_view sv2 = "test";
    const std::string_view sv3 = "TESTING";
    const std::string_view sv4 = "test_different";

    EXPECT_TRUE(equality_checker(sv1, sv2));

    EXPECT_FALSE(equality_checker(sv1, sv3));
    EXPECT_FALSE(equality_checker(sv1, sv4));

    EXPECT_TRUE(equality_checker(sv1, sv1));

    const std::string_view empty1 = "";
    const std::string_view empty2 = "";

    EXPECT_TRUE(equality_checker(empty1, empty2));
    EXPECT_FALSE(equality_checker(sv1, empty1));
}

TEST_F(sv_hash_tests, works_with_unordered_map_for_transparent_lookup) {
    std::unordered_map<std::string_view, int, sv_hash_t, sv_eq_t> my_map;

    my_map["one"]   = 1;
    my_map["two"]   = 2;
    my_map["three"] = 3;

    const std::string_view key_one = "one";

    auto it_one = my_map.find(key_one);

    ASSERT_NE(it_one, my_map.end());
    EXPECT_EQ(it_one->second, 1);

    const std::string_view key_two = "two";

    auto it_two = my_map.find(key_two);

    ASSERT_NE(it_two, my_map.end());
    EXPECT_EQ(it_two->second, 2);

    const std::string_view key_four = "four";

    auto it_four = my_map.find(key_four);

    EXPECT_EQ(it_four, my_map.end());

    EXPECT_EQ(my_map.count(std::string_view("three")), 1);
    EXPECT_EQ(my_map.count(std::string_view("nonexistent")), 0);

    my_map[""] = 0;

    EXPECT_EQ(my_map.count(std::string_view("")), 1);

    auto it_empty = my_map.find(std::string_view(""));

    ASSERT_NE(it_empty, my_map.end());
    EXPECT_EQ(it_empty->second, 0);
}