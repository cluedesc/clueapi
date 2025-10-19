#ifndef CLUEAPI_SHARED_MACROS_HXX
#define CLUEAPI_SHARED_MACROS_HXX

#include <fmt/format.h>

#include <boost/asio/ip/tcp.hpp>

#if defined(_MSC_VER)
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
#else
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
#endif

#ifdef _WIN32
/**
 * @brief A formatter for `boost::asio::ip::tcp::socket::native_handle_type`.
 * * @details This formatter is used to print `boost::asio::ip::tcp::socket::native_handle_type`
 * objects on Windows platform.
 */
template <>
struct fmt::formatter<boost::asio::ip::tcp::socket::native_handle_type>
    : fmt::formatter<std::uintptr_t> {
    template <typename FormatContext>
    auto format(boost::asio::ip::tcp::socket::native_handle_type handle, FormatContext& ctx) const {
        return fmt::formatter<std::uintptr_t>::format(static_cast<std::uintptr_t>(handle), ctx);
    }
};
#endif // _WIN32

#endif // CLUEAPI_SHARED_MACROS_HXX