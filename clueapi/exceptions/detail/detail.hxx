/**
 * @file detail.hxx
 *
 * @brief Internal implementation details for the clueapi exception classes.
 *
 * @note This header is not intended for direct use by end-users.
 */

#ifndef CLUEAPI_EXCEPTIONS_DETAIL_HXX
#define CLUEAPI_EXCEPTIONS_DETAIL_HXX

#include <exception>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/compile.h>
#include <fmt/format.h>

#include "clueapi/shared/macros.hxx"

#include "prefix/prefix.hxx"

/**
 * @namespace clueapi::exceptions::detail
 *
 * @brief Internal data definitions for the clueapi exception classes.
 *
 * @internal
 */
namespace clueapi::exceptions::detail {
    /**
     * @brief A customizable base exception class that automatically prepends a message with a
     * compile-time prefix.
     *
     * @tparam _prefix A `prefix_holder_t` instance that provides the string prefix for the
     * exception message.
     *
     * @details This class inherits from `std::exception` and uses the {fmt} library for
     * message formatting. It is the foundation for all custom exception types in clueapi.
     *
     * @internal
     */
    template <auto _prefix = k_default_prefix>
    struct base_exception_t : std::exception {
        /**
         * @brief Constructs the exception with a formatted message.
         *
         * @param msg A {fmt}-compatible format string.
         * @param args Arguments for the format string.
         */
        template <typename... _args_t>
        CLUEAPI_INLINE base_exception_t(std::string_view msg, _args_t&&... args) noexcept
            : m_what{make(msg, std::forward<_args_t>(args)...)} {
        }

        CLUEAPI_INLINE virtual ~base_exception_t() noexcept = default;

       public:
        /**
         * @brief Returns the full, formatted exception message.
         *
         * @return A C-style string containing the message.
         */
        [[nodiscard]] CLUEAPI_INLINE const char* what() const noexcept override {
            return m_what.c_str();
        }

        /**
         * @brief A static helper to create a formatted string with the class's prefix.
         *
         * @param msg A format-compatible format string.
         * @param args Arguments for the format string.
         *
         * @return The fully formatted string.
         */
        template <typename... _args_t>
        CLUEAPI_INLINE static std::string make(std::string_view msg, _args_t&&... args) noexcept {
            constexpr auto prefix = _prefix.view();

            try {
                if constexpr (sizeof...(_args_t) > 0) {
                    fmt::memory_buffer buf{};

                    if constexpr (prefix.empty()) {
                        fmt::format_to(
                            std::back_inserter(buf),
                            fmt::runtime(msg),
                            std::forward<_args_t>(args)...);
                    } else {
                        fmt::format_to(std::back_inserter(buf), FMT_COMPILE("{}: "), prefix);

                        fmt::format_to(
                            std::back_inserter(buf),
                            fmt::runtime(msg),
                            std::forward<_args_t>(args)...);
                    }

                    return fmt::to_string(buf);
                } else {
                    if constexpr (prefix.empty())
                        return std::string{msg};

                    return fmt::format(FMT_COMPILE("{}: {}"), prefix, msg);
                }
            } catch (...) {
                // ...
            }

            return std::string{};
        }

       private:
        /**
         * @brief The error message.
         */
        std::string m_what;
    };

    /**
     * @brief The default type used to convey error messages within an `expected_t`.
     *
     * @internal
     */
    using message_t = std::string;
} // namespace clueapi::exceptions::detail

#endif // CLUEAPI_EXCEPTIONS_DETAIL_HXX