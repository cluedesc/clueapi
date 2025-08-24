/**
 * @file server.hxx
 *
 * @brief Defines the main server class.
 */

#ifndef CLUEAPI_SERVER_HXX
#define CLUEAPI_SERVER_HXX

#include "client/client.hxx"

/**
 * @namespace clueapi::server
 *
 * @brief The main namespace for the clueapi server.
 *
 * @internal
 */
namespace clueapi::server {
    /**
     * @class c_server
     *
     * @brief Manages the server's lifecycle and client connections.
     *
     * @details This class is responsible for initializing the network acceptor,
     * managing a pool of worker threads, and handling incoming client connections.
     *
     * @internal
     */
    class c_server {
       public:
        /**
         * @brief Constructs a new server instance.
         *
         * @param clueapi A reference to the main clueapi application instance.
         * @param io_ctx_pool A reference to the I/O context pool.
         * @param middleware_chain A reference to the middleware chain.
         * @param cfg The server configuration.
         */
        c_server(
            c_clueapi& clueapi,
            shared::io_ctx_pool_t& io_ctx_pool,
            middleware::next_t& middleware_chain,
            cfg::cfg_t cfg);

        ~c_server();

       public:
        /**
         * @brief Starts the server and begins listening for connections.
         */
        void start();

        /**
         * @brief Stops the server and all associated connections.
         */
        void stop();

       public:
        /**
         * @brief Checks if the server is currently running.
         *
         * @param m The memory order for the atomic load.
         *
         * @return `true` if the server is running, `false` otherwise.
         */
        [[nodiscard]] bool is_running(
            std::memory_order m = std::memory_order_acquire) const noexcept;

       public:
        /**
         * @brief Gets the formatted Keep-Alive timeout string for use in HTTP headers.
         *
         * @return A string view of the timeout value (e.g., "timeout=30").
         */
        [[nodiscard]] CLUEAPI_INLINE std::string_view get_keep_alive_timeout() const noexcept {
            return m_keep_alive_timeout_str;
        }

        /**
         * @brief Sets the formatted Keep-Alive timeout string.
         *
         * @param timeout The new timeout string.
         */
        CLUEAPI_INLINE void set_keep_alive_timeout(const std::string_view& timeout) noexcept {
            m_keep_alive_timeout_str = timeout;
        }

       private:
        friend struct client::client_t;

        friend struct client::detail::response_handler_t;

        [[nodiscard]] CLUEAPI_INLINE c_clueapi& clueapi() const noexcept {
            return m_clueapi;
        }

        [[nodiscard]] CLUEAPI_INLINE middleware::middleware_chain_t& middleware_chain()
            const noexcept {
            return m_middleware_chain;
        }

       private:
        /**
         * @brief A reference to the main clueapi application instance.
         */
        c_clueapi& m_clueapi;

        /**
         * @brief A reference to the middleware chain.
         */
        middleware::middleware_chain_t& m_middleware_chain;

        /**
         * @brief The formatted Keep-Alive timeout string.
         */
        std::string m_keep_alive_timeout_str;

       private:
        /**
         * @class c_impl
         *
         * @brief The internal implementation of the `c_server` class.
         *
         * @internal
         */
        class c_impl;

        /**
         * @brief The internal implementation of the `c_server` class.
         *
         * @internal
         */
        std::unique_ptr<c_impl> m_impl;
    };
} // namespace clueapi::server

#endif // CLUEAPI_SERVER_HXX