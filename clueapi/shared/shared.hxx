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

#if defined(_MSC_VER)
/**
 * @def CLUEAPI_INLINE
 *
 * @brief A macro for forcing function inlining, specific to the MSVC compiler.
 */
#define CLUEAPI_INLINE __forceinline

/**
 * @def CLUEAPI_NOINLINE
 *
 * @brief A macro for preventing function inlining, specific to the MSVC compiler.
 */
#define CLUEAPI_NOINLINE __declspec(noinline)
#else
/**
 * @def CLUEAPI_INLINE
 *
 * @brief A macro for forcing function inlining, used with Clang, GCC, and Intel compilers.
 */
#define CLUEAPI_INLINE __attribute__((always_inline)) inline

/**
 * @def CLUEAPI_NOINLINE
 *
 * @brief A macro for preventing function inlining, used with Clang, GCC, and Intel compilers.
 */
#define CLUEAPI_NOINLINE __attribute__((noinline))
#endif

#ifdef _WIN32
/**
 * @brief A formatter for `boost::asio::ip::tcp::socket::native_handle_type`.
 * 
 * @details This formatter is used to print `boost::asio::ip::tcp::socket::native_handle_type` objects on Windows platform.
 */
template <>
struct fmt::formatter<boost::asio::ip::tcp::socket::native_handle_type>
    : fmt::formatter<std::uintptr_t> {
    template <typename FormatContext>
    auto format(boost::asio::ip::tcp::socket::native_handle_type handle, FormatContext& ctx) const {
        return fmt::formatter<std::uintptr_t>::format(static_cast<std::uintptr_t>(handle), ctx);
    }
};
#endif // _WIN32

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
    CLUEAPI_INLINE std::string sanitize_filename(std::string_view original_name) {
        if (original_name.empty())
            return "untitled";

        std::string sanitized{};

        sanitized.reserve(original_name.length());

        std::copy_if(
            original_name.begin(), original_name.end(), std::back_inserter(sanitized), [](char c) {
                if (std::isalnum(static_cast<unsigned char>(c)))
                    return true;

                switch (c) {
                    case '_':
                    case '-':
                    case '.':
                        return true;

                    default:
                        return false;
                }
            });

        if (sanitized.empty() || sanitized == "." || sanitized == "..")
            return "untitled";

        return sanitized;
    }
} // namespace clueapi::shared

#include "json_traits/json_traits.hxx"

#include "io_ctx_pool/io_ctx_pool.hxx"

#include "non_copy/non_copy.hxx"

#endif // CLUEAPI_SHARED_HXX