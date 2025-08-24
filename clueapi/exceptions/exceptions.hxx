/**
 * @file exceptions.hxx
 *
 * @brief The main public header for the clueapi exception and error-handling system.
 *
 * @note Include this file to use both custom exception types and the `wrap` utilities.
 */

#ifndef CLUEAPI_EXCEPTIONS_HXX
#define CLUEAPI_EXCEPTIONS_HXX

#include "detail/detail.hxx"

/**
 * @namespace clueapi::exceptions
 *
 * @brief The main namespace for the clueapi exception and error-handling system.
 */
namespace clueapi::exceptions {
    /**
     * @brief The default type for error messages in `expected_t`.
     */
    using message_t = detail::message_t;

    /**
     * @brief A general-purpose exception.
     */
    using exception_t = detail::base_exception_t<>;

    /**
     * @brief Exception for errors related to invalid function arguments.
     *
     * @details Has a predefined "Invalid argument" prefix.
     */
    using invalid_argument_t =
        detail::base_exception_t<detail::prefix_holder_t{"Invalid argument"}>;

    /**
     * @brief Exception for general runtime errors that do not fall into other categories.
     *
     * @details Has a predefined "Runtime error" prefix.
     */
    using runtime_error_t = detail::base_exception_t<detail::prefix_holder_t{"Runtime error"}>;

    /**
     * @brief Exception for errors related to input/output operations.
     *
     * @details Has a predefined "I/O error" prefix.
     */
    using io_error_t = detail::base_exception_t<detail::prefix_holder_t{"I/O error"}>;
} // namespace clueapi::exceptions

// Includes the wrap() and wrap_awaitable() utilities.
#include "wrap/wrap.hxx"

#endif // CLUEAPI_EXCEPTIONS_HXX