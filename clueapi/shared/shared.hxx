/**
 * @file shared.hxx
 *
 * @brief Defines shared utilities and types used across the project.
 *
 * @details This file includes common headers, defines convenience macros for compiler-specific
 * features like inlining, and provides type aliases for frequently used data structures
 * and asynchronous programming primitives.
 */

#ifndef CLUEAPI_SHARED_HXX
#define CLUEAPI_SHARED_HXX

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <functional>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>

#include <ankerl/unordered_dense.h>

#include <fmt/format.h>

#include "macros.hxx"

/**
 * @namespace clueapi::shared
 *
 * @brief The main namespace for the clueapi shared utilities.
 */
namespace clueapi::shared {
    /**
     * @brief Type alias for a high-performance hash map.
     *
     * @details Uses `ankerl::unordered_dense::map` for a fast, cache-friendly unordered map.
     *
     * @tparam _key_t The type of the key.
     * @tparam _type_t The type of the value.
     * @tparam _hash_t The type of the hash function.
     * @tparam _eq_t The type of the equality comparison function.
     */
    template <
        typename _key_t,
        typename _type_t,
        typename _hash_t = std::hash<_key_t>,
        typename _eq_t = std::equal_to<_key_t>>
    using unordered_map_t = ankerl::unordered_dense::map<_key_t, _type_t, _hash_t, _eq_t>;

    /**
     * @brief Type alias for an awaitable object from Boost.Asio.
     *
     * @tparam _type_t The result type of the awaitable operation.
     * @tparam _executor The type of the Boost.Asio executor.
     */
    template <typename _type_t, typename _executor = boost::asio::any_io_executor>
    using awaitable_t = boost::asio::awaitable<_type_t, _executor>;

    /**
     * @brief Sanitizes a filename to prevent Path Traversal and remove illegal characters.
     *
     * @details This function uses a whitelist approach, only allowing alphanumeric characters,
     * underscores, hyphens, and dots. It removes all other characters, including slashes,
     * backslashes, and special OS-specific characters. If the resulting filename is empty
     * or consists only of dots, it returns a default safe filename "untitled".
     *
     * @param original_name The original, untrusted filename from the client.
     *
     * @return A safe-to-use filename.
     */
    std::string sanitize_filename(std::string_view original_name);
} // namespace clueapi::shared

#endif // CLUEAPI_SHARED_HXX