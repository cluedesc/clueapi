/**
 * @file prefix.hxx
 *
 * @brief Provides a utility for creating compile-time string literal wrappers.
 *
 * @see clueapi::exceptions::detail::base_exception_t
 */

#ifndef CLUEAPI_EXCEPTIONS_DETAIL_PREFIX_HXX
#define CLUEAPI_EXCEPTIONS_DETAIL_PREFIX_HXX

namespace clueapi::exceptions::detail {
    /**
     * @brief A container to treat a string literal as a template non-type parameter.
     *
     * @tparam _size The size of the string literal array (including the null terminator).
     *
     * @details C++ does not allow raw string literals or `std::string_view` as template
     * non-type parameters. This struct wraps a `char` array, making it a literal type
     * that can be used in templates to pass strings at compile time.
     *
     * @internal
     */
    template <std::uint32_t _size>
    struct prefix_holder_t {
        /**
         * @brief Constructs the holder from a C-style string array.
         */
        CLUEAPI_INLINE constexpr prefix_holder_t() noexcept = default;

        /**
         * @brief Returns the content as a `std::string_view`.
         */
        CLUEAPI_INLINE constexpr prefix_holder_t(const char (&str)[_size]) noexcept {
            std::copy_n(str, _size, m_data);
        }

       public:
        // Allows implicit conversion to std::string_view for convenience.
        CLUEAPI_INLINE constexpr operator std::string_view() const noexcept {
            return view();
        }

        // Allows implicit conversion to std::string_view for convenience.
        CLUEAPI_INLINE constexpr auto operator<=>(const prefix_holder_t&) const noexcept = default;

       public:
        /**
         * @brief Returns the content as a `std::string_view`.
         */
        [[nodiscard]] CLUEAPI_INLINE constexpr std::string_view view() const noexcept {
            if constexpr (_size > 0)
                return {std::data(m_data), _size - 1};
            else
                return {};
        }

       public:
        /**
         * @brief The underlying array of characters.
         */
        char m_data[_size]{};
    };

    /**
     * @brief A default, empty prefix for exceptions that do not need one.
     *
     * @internal
     */
    inline constexpr prefix_holder_t<0> k_default_prefix{};
} // namespace clueapi::exceptions::detail

#endif // CLUEAPI_EXCEPTIONS_DETAIL_PREFIX_HXX