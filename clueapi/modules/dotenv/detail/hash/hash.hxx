/**
 * @file hash.hxx
 *
 * @brief Hashing utilities for the dotenv module.
 */

#ifndef CLUEAPI_MODULES_DOTENV_DETAIL_HASH_HXX
#define CLUEAPI_MODULES_DOTENV_DETAIL_HASH_HXX

#ifdef CLUEAPI_USE_DOTENV_MODULE
namespace clueapi::modules::dotenv::detail {
    /**
     * @brief The basis and prime constants for the hash function.
     *
     * @internal
     */
    inline constexpr auto k_hash_basis = 0x9e3779b9;

    /**
     * @brief The prime constant for the hash function.
     *
     * @internal
     */
    inline constexpr auto k_hash_prime = 0x9f4f2726;

    /**
     * @brief A type alias for the hash function's return type.
     *
     * @internal
     */
    using hash_t = std::size_t;

    /**
     * @brief Computes the hash of a string view at compile time.
     *
     * @internal
     */
    CLUEAPI_INLINE constexpr hash_t ct_hash(std::string_view str) noexcept {
        auto ret = k_hash_basis;

        for (const auto& ch : str) {
            ret = ret * k_hash_prime;

            ret ^= static_cast<hash_t>(ch);
        }

        return ret;
    }

    /**
     * @brief Computes the hash of a string view at runtime.
     *
     * @internal
     */
    CLUEAPI_INLINE hash_t rt_hash(std::string_view str) noexcept {
        auto ret = k_hash_basis;

        for (const auto& ch : str) {
            ret = ret * k_hash_prime;

            ret ^= static_cast<hash_t>(ch);
        }

        return ret;
    }
} // namespace clueapi::modules::dotenv::detail

/**
 * @def ENV_NAME
 *
 * @brief Macro to create a compile-time hash of an environment variable name.
 */
#define ENV_NAME(name) clueapi::modules::dotenv::detail::ct_hash(name)

/**
 * @def ENV_NAME_RT
 *
 * @brief Macro to create a runtime hash of an environment variable name.
 */
#define ENV_NAME_RT(name) clueapi::modules::dotenv::detail::rt_hash(name)

#endif // CLUEAPI_USE_DOTENV_MODULE

#endif // CLUEAPI_MODULES_DOTENV_DETAIL_HASH_HXX