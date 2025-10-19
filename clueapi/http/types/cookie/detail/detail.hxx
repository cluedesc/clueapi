/**
 * @file detail.hxx
 *
 * @brief Internal implementation details for HTTP cookie serialization.
 *
 * @note This header is not intended for direct use by end-users.
 */

#ifndef CLUEAPI_HTTP_TYPES_COOKIE_DETAIL_HXX
#define CLUEAPI_HTTP_TYPES_COOKIE_DETAIL_HXX

#include <cstddef>

/**
 * @namespace clueapi::http::types::detail
 *
 * @brief Internal data definitions for the clueapi HTTP cookie types.
 *
 * @internal
 */
namespace clueapi::http::types::detail {
    /**
     * @brief The default capacity of the memory buffer used for serializing a cookie string.
     *
     * @details This is an optimization to pre-allocate a buffer of a reasonable size to avoid
     * dynamic memory allocations for most common cookies.
     */
    inline constexpr std::size_t k_cookie_buf_capacity = 2048;

    /**
     * @brief A reasonable estimate of the extra characters needed for cookie attributes
     * (e.g., "; Path=/", "; Domain=", etc.), used for buffer pre-allocation.
     */
    inline constexpr std::size_t k_cookie_buf_reserve = 160;
} // namespace clueapi::http::types::detail

#endif // CLUEAPI_HTTP_TYPES_COOKIE_DETAIL_HXX