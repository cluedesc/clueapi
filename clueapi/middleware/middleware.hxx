/**
 * @file middleware.hxx
 *
 * @brief Defines the base components for the middleware system.
 */

#ifndef CLUEAPI_MIDDLEWARE_HXX
#define CLUEAPI_MIDDLEWARE_HXX

#include <functional>
#include <memory>

#include "clueapi/http/types/request/request.hxx"
#include "clueapi/http/types/response/response.hxx"

#include "clueapi/shared/macros.hxx"
#include "clueapi/shared/shared.hxx"

/**
 * @namespace clueapi::middleware
 *
 * @brief The main namespace for the clueapi middleware system.
 */
namespace clueapi::middleware {
    /**
     * @brief A function type representing the next middleware or the final route handler in the
     * chain.
     *
     * @details A middleware's `handle` method receives this function as a parameter. To pass
     * control to the next component in the chain, the middleware must invoke this function. It can
     * choose to do so before or after its own logic, or even not at all (to terminate the request
     * early).
     *
     * @return An awaitable that resolves to the final `http::types::response_t`.
     */
    using next_t =
        std::function<shared::awaitable_t<http::types::response_t>(const http::types::request_t&)>;

    /**
     * @brief A function type representing the middleware chain.
     *
     * @details A middleware's `handle` method receives this function as a parameter. To pass
     * control to the next component in the chain, the middleware must invoke this function. It can
     * choose to do so before or after its own logic, or even not at all (to terminate the request
     * early). This function type is used to represent the middleware chain.
     *
     * @return An awaitable that resolves to the final `http::types::response_t`.
     */
    using middleware_chain_t = next_t;

    /**
     * @class c_base_middleware
     *
     * @brief The abstract base class for all middleware components.
     *
     * @details Users must inherit from this class and override the `handle` method to implement
     * custom middleware logic. The server assembles a chain of these components to process
     * incoming requests before they reach the target route handler.
     */
    class c_base_middleware {
       public:
        /**
         * @brief The core function that executes the middleware's logic.
         *
         * @param request The incoming HTTP request.
         * Middleware can modify the request before passing it to the next handler.
         *
         * @param next A function that passes control to the next middleware in the chain.
         *
         * @return An awaitable that resolves to an `http::types::response_t`.
         *
         * @details The implementation of this method should contain the middleware's logic. It is
         * responsible for calling `co_await next(request)` to continue processing the request
         * chain.
         */
        virtual shared::awaitable_t<http::types::response_t> handle(
            const http::types::request_t& request,

            next_t next) = 0;

        virtual ~c_base_middleware() noexcept = default;

        CLUEAPI_INLINE c_base_middleware() noexcept = default;
    };

    /**
     * @brief A type alias for a shared pointer to a middleware object.
     *
     * @details The framework uses shared pointers to manage
     * the lifetime of middleware instances in the processing chain.
     */
    using middleware_t = std::shared_ptr<c_base_middleware>;
} // namespace clueapi::middleware

#endif // CLUEAPI_MIDDLEWARE_HXX