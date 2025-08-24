/**
 * @file client.hxx
 *
 * @brief Defines the client class that handles a single connection.
 */

#ifndef CLUEAPI_SERVER_CLIENT_HXX
#define CLUEAPI_SERVER_CLIENT_HXX

#include "detail/detail.hxx"
namespace clueapi {
    class c_clueapi;

    /**
     * @namespace clueapi::server::client
     *
     * @brief The main namespace for the clueapi server client.
     *
     * @internal
     */
    namespace server::client {
        /**
         * @struct client_t
         *
         * @brief Manages a single client connection.
         *
         * @details This class is responsible for managing a single client connection.
         * It handles the client's lifecycle, including preparing the socket for use,
         * managing the client's state, and handling incoming requests.
         */
        struct client_t {
            /**
             * @brief Constructs a new client instance.
             */
            client_t(c_server& server, const cfg::cfg_t& cfg);

            /**
             * @brief Destroys the client instance.
             */
            ~client_t();

           public:
            // Copy constructor
            CLUEAPI_INLINE client_t(const client_t&) = delete;

            // Copy assignment operator
            CLUEAPI_INLINE client_t& operator=(const client_t&) = delete;

            // Move constructor
            CLUEAPI_INLINE client_t(client_t&&) = delete;

            // Move assignment operator
            CLUEAPI_INLINE client_t& operator=(client_t&&) = delete;

           public:
            /**
             * @brief Starts the client's lifecycle.
             */
            shared::awaitable_t<void> start();

           public:
            /**
             * @brief Checks if the client is currently idle.
             *
             * @return `true` if the client is idle, `false` otherwise.
             */
            CLUEAPI_INLINE bool prepare_for_connection(boost::asio::ip::tcp::socket&& socket) {
                if (!m_data.is_idle()) {
                    CLUEAPI_LOG_WARNING("Cannot prepare non-idle client for connection");

                    return false;
                }

                return m_data.init(std::move(socket));
            }

            /**
             * @brief Checks if the client is currently idle.
             * @return native handle of the socket if it is open, 0 otherwise.
             */
            CLUEAPI_INLINE std::int32_t get_socket_handle() {
                if (m_data.m_socket && m_data.m_socket->is_open())
                    return m_data.m_socket->native_handle();

                return 0;
            }

            /**
             * @brief Checks if the client is currently idle.
             */
            CLUEAPI_INLINE void return_to_pool() {
                m_data.reset_to_idle();
            }

            /**
             * @brief Checks if the client is currently idle.
             */
            CLUEAPI_INLINE bool is_ready_for_reuse() const {
                return m_data.is_idle();
            }

           private:
            /**
             * @brief The server instance.
             */
            c_server& m_server;

            /**
             * @brief The client configuration.
             */
            const cfg::cfg_t& m_cfg;

            /**
             * @brief The client's data.
             */
            detail::data_t m_data;
        };
    } // namespace server::client
} // namespace clueapi

#endif // CLUEAPI_SERVER_CLIENT_HXX