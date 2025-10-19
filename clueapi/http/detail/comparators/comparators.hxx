/**
 * @file comparators.hxx
 *
 * @brief Defines custom comparators for use within the HTTP module.
 *
 * @note This header is not intended for direct use by end-users.
 */

#ifndef CLUEAPI_HTTP_DETAIL_COMPARATORS_HXX
#define CLUEAPI_HTTP_DETAIL_COMPARATORS_HXX

#include <string_view>

#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "clueapi/shared/macros.hxx"

namespace clueapi::http::detail {
    /**
     * @struct ci_less_t
     *
     * @brief A case-insensitive less-than comparator for `std::string_view`.
     *
     * @internal
     */
    struct ci_less_t {
        /**
         * @brief A type that makes this comparator transparent.
         */
        using is_transparent = void;

        /**
         * @brief Compares two string views case-insensitively.
         *
         * @param lhs The left-hand side string view.
         * @param rhs The right-hand side string view.
         *
         * @return `true` if `lhs` is less than `rhs`, `false` otherwise.
         */
        CLUEAPI_INLINE bool operator()(
            const std::string_view& lhs, const std::string_view& rhs) const noexcept {
            return boost::algorithm::ilexicographical_compare(lhs, rhs);
        }
    };
} // namespace clueapi::http::detail

#endif // CLUEAPI_HTTP_DETAIL_COMPARATORS_HXX