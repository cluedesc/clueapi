/**
 * @file type_traits.hxx
 *
 * @brief Custom type traits for the dotenv module.
 */

#ifndef CLUEAPI_MODULES_DOTENV_DETAIL_TYPE_TRAITS_HXX
#define CLUEAPI_MODULES_DOTENV_DETAIL_TYPE_TRAITS_HXX

#ifdef CLUEAPI_USE_DOTENV_MODULE
namespace clueapi::modules::dotenv::detail {
    template <typename _type_t>
    struct is_string_like : std::false_type {};

    template <>
    struct is_string_like<std::string> : std::true_type {};
    template <>
    struct is_string_like<std::string_view> : std::true_type {};

    template <>
    struct is_string_like<char*> : std::true_type {};
    template <>
    struct is_string_like<const char*> : std::true_type {};
    template <>
    struct is_string_like<volatile char*> : std::true_type {};
    template <>
    struct is_string_like<const volatile char*> : std::true_type {};
    template <std::size_t _size>
    struct is_string_like<char[_size]> : std::true_type {};
    template <std::size_t _size>
    struct is_string_like<const char[_size]> : std::true_type {};

    template <typename _type_t>
    struct is_string_like<_type_t&> : is_string_like<std::remove_cv_t<_type_t>> {};
    template <typename _type_t>
    struct is_string_like<_type_t&&> : is_string_like<std::remove_cv_t<_type_t>> {};
    template <typename _type_t>
    struct is_string_like<const _type_t> : is_string_like<_type_t> {};
    template <typename _type_t>
    struct is_string_like<volatile _type_t> : is_string_like<_type_t> {};
    template <typename _type_t>
    struct is_string_like<const volatile _type_t> : is_string_like<_type_t> {};

    template <typename _type_t>
    inline constexpr bool is_string_like_v = is_string_like<std::decay_t<_type_t>>::value;

    template <typename _type_t>
    inline constexpr bool always_false_v = false;
} // namespace clueapi::modules::dotenv::detail
#endif // CLUEAPI_USE_DOTENV_MODULE

#endif // CLUEAPI_MODULES_DOTENV_DETAIL_TYPE_TRAITS_HXX