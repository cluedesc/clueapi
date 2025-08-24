/**
 * @file json_traits.hxx
 *
 * @brief Defines traits for JSON handling, providing serialization and deserialization.
 *
 * @details This file provides a generic `json_traits_t` struct that can be specialized
 * for different JSON libraries (e.g., nlohmann/json). It is used to encapsulate
 * serialization and deserialization operations.
 */

#ifndef CLUEAPI_SHARED_JSON_TRAITS_HXX
#define CLUEAPI_SHARED_JSON_TRAITS_HXX

namespace clueapi::shared {
#if defined(CLUEAPI_USE_NLOHMANN_JSON) && !defined(CLUEAPI_USE_CUSTOM_JSON)
    /**
     * @struct json_traits_t
     *
     * @brief A traits implementation for JSON handling using the nlohmann/json library.
     *
     * @details Provides static methods to serialize C++ objects into JSON strings
     * and deserialize JSON strings back into C++ objects.
     */
    struct json_traits_t final {
        /**
         * @brief Type alias for a raw JSON string.
         */
        using raw_json_t = std::string;

        /**
         * @brief Type alias for a nlohmann JSON object.
         */
        using json_obj_t = nlohmann::json;

        /**
         * @brief Serializes an object to a JSON string.
         *
         * @tparam _type_t The type of the object to serialize.
         *
         * @param obj The object to serialize.
         *
         * @return The serialized JSON string.
         */
        template <typename _type_t = json_obj_t>
        CLUEAPI_INLINE static raw_json_t serialize(_type_t obj) {
            try {
                nlohmann::json j = std::move(obj);

                if (j.is_null())
                    return "{}";

                return j.dump();
            } catch (...) {
                // ...
            }

            return raw_json_t{};
        }

        /**
         * @brief Deserializes a JSON string to an object.
         *
         * @tparam _type_t The target type of the object.
         *
         * @param json The JSON string to deserialize.
         *
         * @return The deserialized object.
         */
        template <typename _type_t = json_obj_t>
        CLUEAPI_INLINE static _type_t deserialize(const raw_json_t& json) {
            try {
                nlohmann::json j = nlohmann::json::parse(json);

                if (j.is_null())
                    return _type_t{};

                return j.get<_type_t>();
            } catch (...) {
                // ...
            }

            return _type_t{};
        }

        /**
         * @brief Retrieves a value by key from a JSON object.
         *
         * @tparam _type_t The type of the value to retrieve.
         *
         * @param obj The JSON object.
         * @param key The key to look up.
         *
         * @return The value associated with the key.
         */
        template <typename _type_t>
        CLUEAPI_INLINE static _type_t at(const json_obj_t& obj, const std::string_view& key) {
            return obj.at(key).get<_type_t>();
        }
    };
#else
    // ...
#endif // CLUEAPI_USE_NLOHMANN_JSON && !CLUEAPI_USE_CUSTOM_JSON
} // namespace clueapi::shared

#endif // CLUEAPI_SHARED_JSON_TRAITS_HXX