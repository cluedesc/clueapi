/**
 * @file shared.hxx
 *
 * @brief Defines shared non-copyable functions.
 */

#ifndef CLUEAPI_SHARED_NON_COPY_SHARED_HXX
#define CLUEAPI_SHARED_NON_COPY_SHARED_HXX

#include <string_view>

#include "clueapi/shared/macros.hxx"

namespace clueapi::shared::non_copy {
    /**
     * @brief Checks if a character is an ASCII whitespace character.
     *
     * @param c The character to check.
     *
     * @return `true` if the character is a space, tab, carriage return, or newline, `false`
     * otherwise.
     */
    CLUEAPI_INLINE constexpr bool ascii_space(char c) noexcept {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    /**
     * @brief Trims leading ASCII whitespace characters from a string view.
     *
     * @param sv The string view to trim.
     *
     * @return A new string view with leading whitespace removed.
     */
    CLUEAPI_INLINE constexpr std::string_view ltrim(std::string_view sv) noexcept {
        while (!sv.empty() && ascii_space(sv.front()))
            sv.remove_prefix(1);

        return sv;
    }

    /**
     * @brief Trims trailing ASCII whitespace characters from a string view.
     *
     * @param sv The string view to trim.
     *
     * @return A new string view with trailing whitespace removed.
     */
    CLUEAPI_INLINE constexpr std::string_view rtrim(std::string_view sv) noexcept {
        while (!sv.empty() && ascii_space(sv.back()))
            sv.remove_suffix(1);

        return sv;
    }

    /**
     * @brief Trims leading and trailing ASCII whitespace characters from a string view.
     *
     * @param sv The string view to trim.
     *
     * @return A new string view with both leading and trailing whitespace removed.
     */
    CLUEAPI_INLINE constexpr std::string_view trim(std::string_view sv) noexcept {
        return rtrim(ltrim(sv));
    }

    /**
     * @brief Performs a case-insensitive ASCII comparison of two string views.
     *
     * @param a The first string view.
     * @param b The second string view.
     *
     * @return `true` if the two strings are equal, ignoring case, `false` otherwise.
     */
    CLUEAPI_INLINE constexpr bool iequals_ascii(std::string_view a, std::string_view b) noexcept {
        if (a.size() != b.size())
            return false;

        for (size_t i{}; i < a.size(); i++) {
            auto ca = static_cast<unsigned char>(a[i]);
            auto cb = static_cast<unsigned char>(b[i]);

            if (ca == cb)
                continue;

            if ('A' <= ca && ca <= 'Z')
                ca = static_cast<unsigned char>(ca + ('a' - 'A'));

            if ('A' <= cb && cb <= 'Z')
                cb = static_cast<unsigned char>(cb + ('a' - 'A'));

            if (ca != cb)
                return false;
        }

        return true;
    }

    /**
     * @brief Removes a single pair of matching quotes from a string view.
     *
     * @param sv The string view to unquote.
     *
     * @return A new string view with the outermost quotes removed, if they exist.
     */
    CLUEAPI_INLINE constexpr std::string_view unquote(std::string_view sv) noexcept {
        if (sv.size() >= 2u) {
            auto f = sv.front();

            auto b = sv.back();

            if ((f == '"' && b == '"') || (f == '\'' && b == '\'')) {
                sv.remove_prefix(1);
                sv.remove_suffix(1);
            }
        }

        return sv;
    }
} // namespace clueapi::shared::non_copy

#endif // CLUEAPI_SHARED_NON_COPY_SHARED_HXX