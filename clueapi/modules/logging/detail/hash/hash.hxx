/**
 * @file hash.hxx
 *
 * @brief Hashing utilities for the logging module.
 *
 * @details This file provides compile-time and runtime hashing functions for strings,
 * which are used to uniquely identify loggers.
 */

#ifndef CLUEAPI_MODULES_LOGGING_DETAIL_HASH_HXX
#define CLUEAPI_MODULES_LOGGING_DETAIL_HASH_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging::detail {
    /**
     * @brief The basis for the hash function.
     *
     * @internal
     */
    inline constexpr auto k_hash_basis = 0x9e3779b9;

    /**
     * @brief The prime for the hash function.
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
     * @param str The string view to hash.
     *
     * @return The computed hash.
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
     * @param str The string view to hash.
     *
     * @return The computed hash.
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
} // namespace clueapi::modules::logging::detail
#endif // CLUEAPI_USE_LOGGING_MODULE

/**
 * @def LOGGER_NAME
 *
 * @brief Macro to create a compile-time hash of a logger name.
 */
#define LOGGER_NAME(name) clueapi::modules::logging::detail::ct_hash(name)

/**
 * @def LOGGER_NAME_RT
 *
 * @brief Macro to create a runtime hash of a logger name.
 */
#define LOGGER_NAME_RT(name) clueapi::modules::logging::detail::rt_hash(name)

#endif // CLUEAPI_MODULES_LOGGING_DETAIL_HASH_HXX