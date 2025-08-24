/**
 * @file sv_hash.hxx
 *
 * @brief Provides hashing and equality functors for `std::string_view`.
 *
 * @note This header is not intended for direct use by end-users.
 */

#ifndef CLUEAPI_HTTP_DETAIL_SV_HASH_HXX
#define CLUEAPI_HTTP_DETAIL_SV_HASH_HXX

namespace clueapi::http::detail {
    /**
     * @struct sv_hash_t
     *
     * @brief A hashing functor for `std::string_view`.
     *
     * @internal
     */
    struct sv_hash_t {
        /**
         * @brief A type that makes this hashing functor transparent.
         */
        using is_transparent = void;

        /**
         * @brief Computes the hash of a string view.
         *
         * @param sv The string view to hash.
         *
         * @return The hash value.
         */
        CLUEAPI_INLINE std::size_t operator()(std::string_view sv) const noexcept {
            return ankerl::unordered_dense::hash<std::string_view>{}(sv);
        }
    };

    /**
     * @struct sv_eq_t
     *
     * @brief An equality functor for `std::string_view`.
     *
     * @internal
     */
    struct sv_eq_t {
        /**
         * @brief A type that makes this equality functor transparent.
         */
        using is_transparent = void;

        /**
         * @brief Compares two string views.
         *
         * @param lhs The left-hand side string view.
         * @param rhs The right-hand side string view.
         *
         * @return `true` if `lhs` is equal to `rhs`, `false` otherwise.
         */
        CLUEAPI_INLINE bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
            return lhs == rhs;
        }
    };
} // namespace clueapi::http::detail

#endif // CLUEAPI_HTTP_DETAIL_SV_HASH_HXX