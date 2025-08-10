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
    class c_server : public std::enable_shared_from_this<c_server> {
      public:
        /**
         * @brief Constructs a new server instance.
         *
         * @param api A reference to the main clueapi application instance.
         * @param host The hostname or IP address to bind to.
         * @param port The port number to listen on.
         */
        c_server(c_clueapi& api, const std::string& host, std::int32_t port);

        ~c_server();

      public:
        c_server(c_server&&) noexcept;

        c_server& operator=(c_server&&) noexcept;

      public:
        CLUEAPI_INLINE c_server(const c_server&) = delete;

        CLUEAPI_INLINE c_server& operator=(const c_server&) = delete;

      public:
        /**
         * @brief Starts the server and begins listening for connections.
         *
         * @param workers The number of worker threads to spawn.
         */
        void start(std::uint32_t workers);

        /**
         * @brief Stops the server and all associated threads and connections.
         */
        void stop();

      public:
        /**
         * @brief Gets the `io_context` associated with the server.
         *
         * @return A reference to the `boost::asio::io_context`.
         */
        boost::asio::io_context& io_ctx();

        /**
         * @brief Checks if the server is currently running.
         *
         * @param m The memory order for the atomic load.
         *
         * @return `true` if the server is running, `false` otherwise.
         */
        bool is_running(std::memory_order m = std::memory_order_acquire) const;

      public:
        /**
         * @brief Gets the formatted Keep-Alive timeout string for use in HTTP headers.
         *
         * @return A string view of the timeout value (e.g., "timeout=30").
         */
        CLUEAPI_INLINE std::string_view get_keep_alive_timeout() const { return m_keep_alive_timeout_str; }

        /**
         * @brief Sets the formatted Keep-Alive timeout string.
         *
         * @param timeout The new timeout string.
         */
        CLUEAPI_INLINE void set_keep_alive_timeout(const std::string_view& timeout) {
            m_keep_alive_timeout_str = timeout;
        }

      private:
        friend struct client_t;

        /**
         * @brief The main asynchronous loop for accepting new client connections.
         */
        shared::awaitable_t<void> do_accept();

        /**
         * @brief Returns a client object to the internal pool for reuse.
         *
         * @param client A pointer to the client_t object to return.
         */
        void return_client_to_pool(client_t* client);

      private:
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
}

#endif // CLUEAPI_SERVER_HXX