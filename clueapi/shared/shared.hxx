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

#ifdef __MSVC_COMPILER__
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
#elif __CLANG_COMPILER__ || __GCC_COMPILER__ || __INTEL_COMPILER__
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
#else
/**
 * @def CLUEAPI_INLINE
 * 
 * @brief A standard-compliant macro for suggesting function inlining.
 */
#define CLUEAPI_INLINE inline

/**
 * @def CLUEAPI_NOINLINE
 * 
 * @brief A placeholder macro for compilers that do not support explicit no-inline directives.
 */
#define CLUEAPI_NOINLINE
#endif // ...

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
        typename _key_t, typename _type_t, typename _hash_t = std::hash<_key_t>, typename _eq_t = std::equal_to<_key_t>>
    using unordered_map = ankerl::unordered_dense::map<_key_t, _type_t, _hash_t, _eq_t>;

    /**
     * @brief Type alias for an awaitable object from Boost.Asio.
     *
     * @tparam _type_t The result type of the awaitable operation.
     * @tparam _executor The type of the Boost.Asio executor.
     */
    template <typename _type_t, typename _executor = boost::asio::any_io_executor>
    using awaitable_t = boost::asio::awaitable<_type_t, _executor>;
}

#include "json_traits/json_traits.hxx"

#endif // CLUEAPI_SHARED_HXX