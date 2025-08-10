/**
 * @file clueapi.hxx
 *
 * @brief The main public header for the clueapi library.
 *
 * @details This file serves as the primary entry point for using the clueapi library.
 * It includes all necessary sub-modules, defines the main application class `c_clueapi`,
 * and exposes global module instances like logging and dotenv when they are enabled.
 */

#ifndef CLUEAPI_HXX
#define CLUEAPI_HXX

#include "shared/shared.hxx"

#include "exceptions/exceptions.hxx"

#include "modules/modules.hxx"

namespace clueapi {
#ifdef CLUEAPI_USE_LOGGING_MODULE
    /**
     * @brief The global instance of the logging module.
     */
    extern const std::unique_ptr<modules::logging::c_logging> g_logging;
#endif // CLUEAPI_USE_LOGGING_MODULE

#ifdef CLUEAPI_USE_DOTENV_MODULE
    /**
     * @brief The global instance of the dotenv module.
     */
    extern const std::unique_ptr<modules::dotenv::c_dotenv> g_dotenv;
#endif // CLUEAPI_USE_DOTENV_MODULE
}

#include "modules/macros.hxx"

#include "http/http.hxx"

#include "cfg/cfg.hxx"

#include "route/route.hxx"

#include "middleware/middleware.hxx"

#include "server/server.hxx"

/**
 * @namespace clueapi
 *
 * @brief The main namespace for the clueapi.
 */
namespace clueapi {
    /**
     * @brief Type alias for the main configuration structure.
     */
    using cfg_t = cfg::cfg_t;

    /**
     * @class c_clueapi
     *
     * @brief The main application class for the clueapi server.
     *
     * @details This class orchestrates the entire server lifecycle. It is responsible for
     * initializing the server with a given configuration, managing routes, handling
     * shutdown signals, and processing incoming requests through a middleware chain.
     */
    class c_clueapi {
      public:
        c_clueapi();

        ~c_clueapi();

      public:
        /**
         * @brief Starts the server with the specified configuration.
         *
         * @param cfg The configuration settings for the server.
         */
        void start(cfg_t cfg);

        /**
         * @brief Blocks the calling thread until the server is stopped.
         *
         * @details This is useful for keeping the main application alive while the server runs.
         */
        void wait();

        /**
         * @brief Initiates a graceful shutdown of the server.
         */
        void stop();

      private:
        /**
         * @brief A variant representing either a synchronous or an asynchronous route handler.
         */
        using route_t = std::variant<
            std::function<http::types::response_t(http::ctx_t)>,
            std::function<shared::awaitable_t<http::types::response_t>(http::ctx_t)>>;

      public:
        /**
         * @brief Adds a new route for a specific HTTP method and path.
         *
         * @param method The HTTP method (e.g., GET, POST).
         * @param path The URL path for the route.
         * @param handler The function (sync or async) that will handle requests to this route.
         */
        void add_method(http::types::method_t::e_method method, http::types::path_t path, route_t&& handler);

        /**
         * @brief Adds a new middleware to the middleware chain.
         *
         * @param middleware The middleware to be added to the chain.
         */
        void add_middleware(middleware::middleware_t middleware);

        /**
         * @brief Gets the current configuration of the server.
         *
         * @return A const reference to the configuration object.
         */
        const cfg_t& cfg() const;

        /**
         * @brief Checks if the clueapi is currently running.
         *
         * @param m The memory order for the atomic load operation.
         *
         * @return `true` if the server is running, `false` otherwise.
         */
        bool is_running(std::memory_order m = std::memory_order_acquire) const;

      private:
        friend struct server::client_t;

        /**
         * @brief The internal request handler that processes a request through the middleware and routing system.
         *
         * @param request The incoming HTTP request.
         * @param response The HTTP response to be populated.
         *
         * @return An awaitable resolving to `true` if the request was handled, `false` otherwise.
         */
        shared::awaitable_t<bool> handle_request(http::types::request_t& request, http::types::response_t& response);

      private:
        /**
         * @class c_impl
         *
         * @brief The internal implementation of the `c_clueapi` class.
         *
         * @internal
         */
        class c_impl;

        /**
         * @brief The internal implementation of the `c_clueapi` class.
         * 
         * @internal
         */
        std::unique_ptr<c_impl> m_impl{};
    };

    /**
     * @brief Constructs a new instance of the `c_clueapi` class.
     * 
     * @details This is the main entry point for using the clueapi library.
     */
    CLUEAPI_INLINE c_clueapi api() { return c_clueapi(); }
}

#endif // CLUEAPI_HXX