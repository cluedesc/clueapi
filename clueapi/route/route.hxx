/**
 * @file route.hxx
 *
 * @brief Defines the route handlers for the application.
 */

#ifndef CLUEAPI_ROUTE_HXX
#define CLUEAPI_ROUTE_HXX

#include "detail/detail.hxx"

/**
 * @namespace clueapi::route
 *
 * @brief The main namespace for the clueapi route handlers.
 */
namespace clueapi::route {
    /**
     * @brief A base class for routes.
     *
     * @details This class provides a polymorphic interface for route handlers. The framework
     * automatically determines whether to invoke the synchronous `handle` method or the
     * asynchronous `handle_awaitable` method based on the type of the handler provided
     * during route registration (i.e., whether it is a regular function or a C++20 coroutine).
     */
    struct base_route_t {
        virtual ~base_route_t() noexcept = default;

        CLUEAPI_INLINE base_route_t() noexcept = default;

        CLUEAPI_INLINE base_route_t(const base_route_t&) noexcept = delete;

        CLUEAPI_INLINE base_route_t& operator=(const base_route_t&) noexcept = delete;

        CLUEAPI_INLINE base_route_t(base_route_t&&) noexcept = default;

        CLUEAPI_INLINE base_route_t& operator=(base_route_t&&) noexcept = default;

        /**
         * @brief Handles a request.
         *
         * @param ctx The request context.
         *
         * @return The response.
         */
        virtual http::types::response_t handle(http::ctx_t ctx) = 0;

        /**
         * @brief Handles a request asynchronously.
         *
         * @param ctx The request context.
         *
         * @return The response.
         */
        virtual shared::awaitable_t<http::types::response_t> handle_awaitable(http::ctx_t ctx) = 0;

        /**
         * @brief Checks if the route is awaitable.
         *
         * @return `true` if the route is awaitable, `false` otherwise.
         */
        [[nodiscard]] virtual bool is_awaitable() const noexcept = 0;
    };

    /**
     * @brief A route handler.
     *
     * @tparam _handler_t The handler type.
     */
    template <typename _handler_t>
    struct route_t : base_route_t {
        CLUEAPI_INLINE constexpr route_t() noexcept = default;

        /**
         * @brief Constructs a route with the given parameters.
         *
         * @param method The HTTP method.
         * @param path The path.
         * @param handler The handler.
         */
        CLUEAPI_INLINE route_t(
            const http::types::method_t::e_method& method,
            http::types::path_t path,

            _handler_t&& handler) noexcept
            : m_handler(std::move(handler)), m_path(std::move(path)), m_method(method) {
        }

       public:
        // Copy constructor
        CLUEAPI_INLINE route_t(const route_t&) = delete;

        // Copy assignment operator
        CLUEAPI_INLINE route_t& operator=(const route_t&) noexcept = delete;

        // Move constructor
        CLUEAPI_INLINE route_t(route_t&&) noexcept = default;

        // Move assignment operator
        CLUEAPI_INLINE route_t& operator=(route_t&&) noexcept = default;

       public:
        /**
         * @brief Handles a request.
         *
         * @param ctx The request context.
         *
         * @return The response.
         */
        CLUEAPI_INLINE http::types::response_t handle(http::ctx_t ctx) override {
            if constexpr (detail::handler_c<_handler_t>) {
                return m_handler(std::move(ctx));
            } else
                throw exceptions::exception_t("Asynchronous handler called synchronously");
        }

        /**
         * @brief Handles a request asynchronously.
         *
         * @param ctx The request context.
         *
         * @return The response.
         */
        CLUEAPI_NOINLINE shared::awaitable_t<http::types::response_t> handle_awaitable(
            http::ctx_t ctx) override {
            if constexpr (detail::handler_awaitable_c<_handler_t>) {
                co_return co_await m_handler(std::move(ctx));
            } else
                co_return handle(std::move(ctx));
        }

        /**
         * @brief Checks if the route is awaitable.
         *
         * @return `true` if the route is awaitable, `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE bool is_awaitable() const noexcept override {
            return detail::handler_awaitable_c<_handler_t>;
        }

       public:
        /**
         * @brief Checks if the route is awaitable at compile time.
         */
        static constexpr bool k_is_awaitable = detail::handler_awaitable_c<_handler_t>;

       private:
        /**
         * @brief The handler function.
         */
        _handler_t m_handler{};

        /**
         * @brief The path of the route.
         */
        http::types::path_t m_path;

        /**
         * @brief The HTTP method.
         */
        http::types::method_t::e_method m_method{};
    };
} // namespace clueapi::route

#endif // CLUEAPI_ROUTE_HXX