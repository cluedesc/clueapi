#include <gtest/gtest.h>

#include "clueapi/http/detail/comparators/comparators.hxx"

using clueapi::http::detail::ci_less_t;

class ci_less_tests : public ::testing::Test {
  protected:
    ci_less_t case_insensitive_less;
};

TEST_F(ci_less_tests, returns_true_for_lesser_strings) {
    EXPECT_TRUE(case_insensitive_less("apple", "Banana"));
    EXPECT_TRUE(case_insensitive_less("cat", "DOG"));
    EXPECT_TRUE(case_insensitive_less("a", "b"));
}

TEST_F(ci_less_tests, returns_false_for_greater_strings) {
    EXPECT_FALSE(case_insensitive_less("Banana", "apple"));
    EXPECT_FALSE(case_insensitive_less("DOG", "cat"));
    EXPECT_FALSE(case_insensitive_less("b", "a"));
}

TEST_F(ci_less_tests, returns_false_for_equal_strings) {
    EXPECT_FALSE(case_insensitive_less("Grape", "grape"));
    EXPECT_FALSE(case_insensitive_less("TEST", "test"));
    EXPECT_FALSE(case_insensitive_less("EQUAL", "EQUAL"));
}

TEST_F(ci_less_tests, handles_empty_strings) {
    EXPECT_TRUE(case_insensitive_less("", "a"));
    EXPECT_FALSE(case_insensitive_less("a", ""));
    EXPECT_FALSE(case_insensitive_less("", ""));
}

TEST_F(ci_less_tests, handles_substrings) {
    EXPECT_TRUE(case_insensitive_less("sub", "substring"));
    EXPECT_FALSE(case_insensitive_less("substring", "sub"));
}