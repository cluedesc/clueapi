/**
 * @file dotenv.hxx
 *
 * @brief The main public header for the clueapi dotenv module.
 *
 * @details This file provides the `c_dotenv` class, which allows for loading
 * and accessing environment variables from a `.env` file.
 */

#ifndef CLUEAPI_MODULES_DOTENV_HXX
#define CLUEAPI_MODULES_DOTENV_HXX

#include <charconv>
#include <string>
#include <system_error>
#include <type_traits>

#include <boost/algorithm/string.hpp>

#include "clueapi/shared/macros.hxx"

#include "detail/container/container.hxx"
#include "detail/hash/hash.hxx"
#include "detail/type_traits/type_traits.hxx"

#ifdef CLUEAPI_USE_DOTENV_MODULE
namespace clueapi::modules::dotenv {
    /**
     * @class c_dotenv
     *
     * @brief Manages the loading and retrieval of configuration variables from a file.
     *
     * @details This class parses a specified `.env` file, stores the key-value pairs,
     * and provides a type-safe interface for retrieving values. Keys are hashed for
     * efficient lookups.
     */
    class c_dotenv {
       public:
        CLUEAPI_INLINE c_dotenv() noexcept = default;

        CLUEAPI_INLINE ~c_dotenv() noexcept {
            destroy();
        }

       public:
        /**
         * @brief Loads and parses the specified environment file.
         *
         * @param filename The path to the `.env` file.
         * @param trim_values If `true`, whitespace will be trimmed from the start and end of
         * values.
         */
        void load(std::string filename, bool trim_values = false);

        void destroy();

       public:
        /**
         * @brief Gets the number of loaded environment variables.
         *
         * @return The total number of key-value pairs.
         */
        [[nodiscard]] CLUEAPI_INLINE std::size_t size() const noexcept {
            return m_container.size();
        }

        /**
         * @brief Checks if a variable with the given key exists.
         *
         * @param key The hashed key (`ENV_NAME` or `ENV_NAME_RT`) of the variable.
         *
         * @return `true` if the key exists, `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE bool contains(const detail::hash_t& key) const noexcept {
            return m_container.contains(key);
        }

        template <typename _value_t>
        CLUEAPI_INLINE _value_t at(const detail::hash_t& key) const {
            return at<_value_t>(key, _value_t{});
        }

        /**
         * @brief Retrieves the value associated with a key, with a default fallback.
         *
         * @tparam _value_t The desired type of the value (e.g., `std::string`, `int`, `bool`).
         *
         * @param key The hashed key (`ENV_NAME` or `ENV_NAME_RT`) of the variable.
         * @param default_value The value to return if the key is not found.
         *
         * @return The variable's value converted to `_value_t`, or `default_value`.
         */
        template <typename _value_t>
        CLUEAPI_INLINE _value_t at(const detail::hash_t& key, const _value_t& default_value) const {
            if (!contains(key))
                return default_value;

            const auto& value_str = m_container.at(key);

            if constexpr (detail::is_string_like_v<_value_t>) {
                return value_str;
            } else if constexpr (std::is_same_v<_value_t, bool>) {
                auto value = boost::to_lower_copy(value_str);

                return value == "true";
            } else if constexpr (
                std::is_integral_v<_value_t> || std::is_floating_point_v<_value_t>) {
                _value_t ret{};

                auto [ptr, ec] =
                    std::from_chars(value_str.data(), value_str.data() + value_str.size(), ret);

                if (ec == std::errc())
                    return ret;

                return default_value;
            } else
                static_assert(detail::always_false_v<_value_t>, "Unsupported type");
        }

       private:
        void parse();

       private:
        /**
         * @brief Whether to trim values.
         */
        bool m_trim_values{false};

        /**
         * @brief The filename of the dotenv file.
         */
        std::string m_filename;

        /**
         * @brief The internal container.
         */
        detail::container_t<detail::hash_t, std::string> m_container;
    };

    /**
     * @brief A dispatch helper to allow calling `get` on both pointers and references to
     * `c_dotenv`.
     */
    template <typename _value_t, typename _env_t>
    CLUEAPI_INLINE _value_t get_dispatch(
        _env_t&& env, const detail::hash_t& key, const _value_t& default_value = _value_t{}) {
        if constexpr (std::is_pointer_v<std::remove_reference_t<_env_t>>) {
            return env->template at<_value_t>(key, default_value);
        } else {
            return env.template at<_value_t>(key, default_value);
        }
    }
} // namespace clueapi::modules::dotenv

/**
 * @def CLUEAPI_DOTENV_GET_IMPL
 *
 * @brief A macro to simplify dispatching calls to the global `c_dotenv` instance.
 */
#define CLUEAPI_DOTENV_GET_IMPL(main_dotenv, key, type, default_val) \
    (::clueapi::modules::dotenv::get_dispatch<type>(main_dotenv, key, default_val))

#endif // CLUEAPI_USE_DOTENV_MODULE

#endif // CLUEAPI_MODULES_DOTENV_HXX