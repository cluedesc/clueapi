#if !defined(CLUEAPI_USE_NLOHMANN_JSON)
#define CLUEAPI_USE_NLOHMANN_JSON
#endif // !CLUEAPI_USE_NLOHMANN_JSON

#ifdef CLUEAPI_USE_CUSTOM_JSON
#undef CLUEAPI_USE_CUSTOM_JSON
#endif // CLUEAPI_USE_CUSTOM_JSON

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "clueapi/shared/json_traits/json_traits.hxx"

#include <nlohmann/json.hpp>

using json_traits = clueapi::shared::json_traits_t;

using json = nlohmann::json;

class json_traits_tests : public ::testing::Test {
  protected:
    void SetUp() override {
        sample_json_obj_ = {{"name", "test-user"}, {"id", 123}, {"active", true}, {"scores", {10, 20, 30}}};

        expected_json_str_ = R"({"active":true,"id":123,"name":"test-user","scores":[10,20,30]})";
    }

    json sample_json_obj_;

    std::string expected_json_str_;
};

TEST_F(json_traits_tests, serializes_json_object_to_string) {
    auto result_str = json_traits::serialize(sample_json_obj_);

    EXPECT_EQ(json::parse(result_str), sample_json_obj_);
}

TEST_F(json_traits_tests, serialize_handles_empty_object) {
    json null_obj;

    auto result_str = json_traits::serialize(null_obj);

    EXPECT_EQ(result_str, "{}");
}

TEST_F(json_traits_tests, deserializes_string_to_json_object) {
    auto result_obj = json_traits::deserialize(expected_json_str_);

    ASSERT_FALSE(result_obj.is_null());
    EXPECT_EQ(result_obj, sample_json_obj_);
}

TEST_F(json_traits_tests, deserialize_returns_null_for_invalid_json) {
    const std::string invalid_json_str = R"({"key": "value")";

    auto result_obj = json_traits::deserialize(invalid_json_str);

    EXPECT_TRUE(result_obj.is_null());
}

TEST_F(json_traits_tests, deserialize_returns_null_for_type_mismatch) {
    const std::string json_str = R"({"value": 123})";

    auto result_obj = json_traits::deserialize<std::vector<int>>(json_str);

    EXPECT_TRUE(result_obj.empty());
}

TEST_F(json_traits_tests, at_retrieves_value_by_key) {
    EXPECT_EQ(json_traits::at<std::string>(sample_json_obj_, "name"), "test-user");
    EXPECT_EQ(json_traits::at<int>(sample_json_obj_, "id"), 123);

    EXPECT_TRUE(json_traits::at<bool>(sample_json_obj_, "active"));

    const std::vector<int> expected_scores = {10, 20, 30};

    EXPECT_EQ(json_traits::at<std::vector<int>>(sample_json_obj_, "scores"), expected_scores);
}

TEST_F(json_traits_tests, at_throws_for_nonexistent_key) {
    EXPECT_THROW({ json_traits::at<std::string>(sample_json_obj_, "nonexistent-key"); }, json::out_of_range);
}

TEST_F(json_traits_tests, at_throws_for_type_mismatch) {
    EXPECT_THROW({ json_traits::at<int>(sample_json_obj_, "name"); }, json::type_error);
}