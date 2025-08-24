/**
 * @file detail.hxx
 *
 * @brief Internal implementation details for the HTTP module.
 *
 * @note This header is not intended for direct use by end-users.
 */

#ifndef CLUEAPI_HTTP_DETAIL_HXX
#define CLUEAPI_HTTP_DETAIL_HXX

#include "comparators/comparators.hxx"

#include "sv_hash/sv_hash.hxx"

/**
 * @namespace clueapi::http::detail
 *
 * @brief Internal data definitions for the clueapi HTTP module.
 *
 * @internal
 */
namespace clueapi::http::detail {
    /**
     * @brief Decodes a URL-encoded string.
     *
     * @param str The URL-encoded string view to decode.
     * @return The decoded string.
     *
     * @details This function processes a string and converts URL-encoded sequences back to their
     * original characters. It handles:
     * - `%xx` hex-encoded characters.
     * - `+` characters, which are converted to spaces.
     */
    CLUEAPI_INLINE std::string url_decode(const std::string_view& str) {
        std::string decoded_str{};

        decoded_str.reserve(str.length());

        for (std::size_t i{}; i < str.length(); i++) {
            if (str[i] == '%' && i + 2 < str.length()) {
                unsigned char value{};

                const auto* start_ptr = str.data() + i + 1;
                const auto* end_ptr = start_ptr + 2;

                auto [ptr, ec] = std::from_chars(start_ptr, end_ptr, value, 16);

                if (ec == std::errc{}) {
                    decoded_str += static_cast<char>(value);

                    i += 2;
                } else
                    decoded_str += '%';
            } else if (str[i] == '+') {
                decoded_str += ' ';
            } else
                decoded_str += str[i];
        }

        return decoded_str;
    }
} // namespace clueapi::http::detail

#endif // CLUEAPI_HTTP_DETAIL_HXX