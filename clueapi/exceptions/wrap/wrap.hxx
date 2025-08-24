/**
 * @file wrap.hxx
 *
 * @brief Provides functions to bridge exception-based code with `tl::expected`-based error
 * handling.
 *
 * @details This file contains the core logic for safely executing code that may throw
 * exceptions and wrapping the outcome into a `tl::expected` object. This is especially
 * useful for handling errors in asynchronous code (coroutines) where exceptions are problematic.
 */

#ifndef CLUEAPI_EXCEPTIONS_WRAP_HXX
#define CLUEAPI_EXCEPTIONS_WRAP_HXX

namespace clueapi::exceptions {
    /**
     * @brief Represents a result that is either an expected value or an error.
     *
     * @tparam _ret_t The type of the expected value. Defaults to `void`.
     * @tparam _message_t The type used to store the error. Defaults to `message_t`.
     */
    template <typename _ret_t = void, typename _message_t = message_t>
    using expected_t = tl::expected<_ret_t, _message_t>;

    /**
     * @brief An awaitable that, when awaited, resolves to an `expected_t`.
     */
    template <typename _ret_t = void, typename _message_t = message_t>
    using expected_awaitable_t = shared::awaitable_t<expected_t<_ret_t, _message_t>>;

    /**
     * @brief A factory function to create a `tl::unexpected` instance.
     */
    template <typename _message_t = message_t>
    CLUEAPI_INLINE tl::unexpected<_message_t> make_unexpected(_message_t msg = {}) noexcept {
        return tl::make_unexpected(std::move(msg));
    }

    /**
     * @brief Executes a callable and safely wraps any thrown std::exception into a `tl::expected`.
     *
     * @tparam _ret_t The expected return type of the callable.
     *
     * @param callable The synchronous function or lambda to execute.
     * @param ctx A context string to prepend to any resulting error message for diagnostics.
     *
     * @return An `expected_t` containing the successful result or a formatted error message.
     *
     * @details This function is a bridge for calling code (e.g., third-party libraries)
     * that throws exceptions and converting the outcome into a modern, explicit error-handling
     * model.
     */
    template <typename _ret_t = void, typename _callable_t>
    CLUEAPI_INLINE expected_t<_ret_t> wrap(_callable_t&& callable, std::string_view ctx) {
        try {
            if constexpr (std::is_void_v<_ret_t>) {
                (void)std::forward<_callable_t>(callable)();

                return expected_t<_ret_t>{};
            } else
                return std::forward<_callable_t>(callable)();
        } catch (const std::exception& e) {
            return make_unexpected(fmt::format(FMT_COMPILE("{}: {}"), ctx, e.what()));
        } catch (...) {
            return make_unexpected(fmt::format(FMT_COMPILE("{}: unknown"), ctx));
        }
    }

    /**
     * @brief Executes an awaitable and safely wraps any thrown std::exception into an
     * `expected_awaitable_t`.
     *
     * @tparam _ret_t The expected return type of the awaitable.
     *
     * @param awaitable The coroutine or awaitable object to execute.
     * @param ctx A context string to prepend to any resulting error message for diagnostics.
     *
     * @return An awaitable that resolves to an `expected_t` containing the result or a formatted
     * error.
     *
     * @details This is the asynchronous counterpart to `wrap()`. It is essential for robust
     * error handling in coroutines, as exceptions do not propagate safely across suspension points.
     * An exception thrown within a coroutine after it has been suspended can lead to program
     * termination rather than being caught by a traditional try/catch block in the calling
     * function. This wrapper ensures all exceptions are caught and converted to an error value.
     */
    template <typename _ret_t = void, typename _awaitable_t>
    CLUEAPI_NOINLINE expected_awaitable_t<_ret_t> wrap_awaitable(
        _awaitable_t awaitable, std::string_view ctx) {
        try {
            if constexpr (std::is_void_v<_ret_t>) {
                (void)co_await std::move(awaitable);

                co_return expected_t<_ret_t>{};
            } else {
                if constexpr (std::is_invocable_v<_awaitable_t>) {
                    auto result = co_await std::move(awaitable)();

                    co_return expected_t<_ret_t>{std::move(result)};
                } else {
                    auto result = co_await std::move(awaitable);

                    co_return expected_t<_ret_t>{std::move(result)};
                }
            }
        } catch (const std::exception& e) {
            co_return make_unexpected(fmt::format(FMT_COMPILE("{}: {}"), ctx, e.what()));
        } catch (...) {
            co_return make_unexpected(fmt::format(FMT_COMPILE("{}: unknown"), ctx));
        }
    }
} // namespace clueapi::exceptions

#endif // CLUEAPI_EXCEPTIONS_WRAP_HXX