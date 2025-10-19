/**
 * @file cfg.hxx
 *
 * @brief Defines the main configuration structure for the clueapi library.
 *
 * @note This file provides default values for all configuration parameters.
 */

#ifndef CLUEAPI_CFG_HXX
#define CLUEAPI_CFG_HXX

#include <chrono>
#include <cstdint>
#include <string>

#include "clueapi/http/types/response_class/response_class.hxx"

#include "clueapi/http/multipart/detail/types/cfg/cfg.hxx"

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include "logging/logging.hxx"
#endif // CLUEAPI_USE_LOGGING_MODULE

/**
 * @namespace clueapi::cfg
 *
 * @brief Contains all configuration-related structures and types for the clueapi.
 */
namespace clueapi::cfg {
    /**
     * @struct cfg_t
     *
     * @brief Aggregates all configuration settings for the clueapi server.
     */
    struct cfg_t {
        /**
         * @brief Hostname or IP address for the server to listen on. E.g., "localhost",
         * "127.0.0.1", "0.0.0.0".
         */
        std::string m_host{"localhost"};

        /**
         * @brief The network port to listen on.
         */
        std::string m_port{"8080"};

        /**
         * @brief Number of worker threads for the server to handle incoming requests.
         *
         * @note A good starting value is the number of CPU cores.
         */
        std::int32_t m_workers{2};

        /**
         * @struct server_t
         *
         * @brief Configuration specific to the underlying server implementation.
         */
        struct server_t {
            /**
             * @brief Path to a temporary directory for file uploads and other transient data.
             *
             * @warning The server process must have write permissions for this directory.
             */
            std::string m_tmp_dir{"/tmp/clueapi"};

            /**
             * @brief The maximum time to wait for client connections to finish before destroying
             * them.
             */
            std::chrono::seconds m_deadline_for_destroying_clients{5};

            /**
             * @struct client_t
             *
             * @brief Configuration specific to the client.
             */
            struct client_t {
                /**
                 * @brief Default buffer capacity for the client in bytes.
                 */
                std::size_t m_buffer_capacity{64ull * 1024u};
            } m_client{};

            /**
             * @struct acceptor_t
             *
             * @brief Low-level configuration for the connection acceptor socket.
             */
            struct acceptor_t {
                /**
                 * @brief Maximum number of concurrent connections the acceptor will accept.
                 */
                std::size_t m_max_connections{2048};

                /**
                 * @brief If true, the acceptor socket will be non-blocking. Recommended for high
                 * performance.
                 */
                bool m_nonblocking{true};

                /**
                 * @brief If true, allows the socket to be bound to an address that is already in
                 * use (SO_REUSEADDR).
                 */
                bool m_reuse_address{true};

                /**
                 * @brief If true, allows multiple sockets to bind to the same address and port
                 * (SO_REUSEPORT). Useful for multi-process servers.
                 */
                bool m_reuse_port{true};

                /**
                 * @brief If true, enables TCP Fast Open (TFO) to speed up subsequent connections.
                 * Requires OS support.
                 */
                bool m_tcp_fast_open{true};
            } m_acceptor{};
        } m_server{};

        /**
         * @struct http_t
         *
         * @brief HTTP protocol-specific settings.
         */
        struct http_t {
            /**
             * @brief The default Content-Type class for responses if not specified otherwise.
             */
            http::types::e_response_class m_def_response_class{
                http::types::e_response_class::plain};

            /**
             * @brief If true, enables HTTP Keep-Alive connections.
             */
            bool m_keep_alive_enabled{true};

            /**
             * @brief Timeout for HTTP Keep-Alive connections.
             *
             * @note The server will close an idle connection after this period. A value of 0 may
             * disable the timeout.
             */
            std::chrono::milliseconds m_keep_alive_timeout{std::chrono::seconds{30}};

            /**
             * @brief Configuration for the multipart/form-data parser.
             */
            http::multipart::detail::cfg_t m_multipart_parser_cfg{};

            /**
             * @brief Maximum size of a full HTTP request in bytes, including headers and body.
             */
            std::size_t m_max_request_size{104857600u};

            /**
             * @brief Maximum allowed size for the headers part of an HTTP request, in bytes.
             */
            std::size_t m_max_hdrs_request_size{16384u};

            /**
             * @brief The size of the buffer chunk used when reading/parsing a request.
             */
            std::size_t m_chunk_size{131072};
        } m_http{};

        /**
         * @struct socket_t
         *
         * @brief Low-level TCP socket options for each connection.
         */
        struct socket_t {
            /**
             * @brief If true, disables Nagle's algorithm (TCP_NODELAY). Reduces latency for small
             * packets.
             */
            bool m_tcp_no_delay{true};

            /**
             * @brief If true, enables TCP keep-alive probes to detect dead connections.
             */
            bool m_tcp_keep_alive{true};

            /**
             * @brief Size of the TCP receive buffer (SO_RCVBUF) in bytes.
             */
            std::size_t m_rcv_buf_size{524288};

            /**
             * @brief Size of the TCP send buffer (SO_SNDBUF) in bytes.
             */
            std::size_t m_snd_buf_size{524288};

            /**
             * @brief Timeout for socket operations. A value of 0 may disable the timeout.
             */
            std::chrono::milliseconds m_timeout{std::chrono::seconds{15}};
        } m_socket{};

#ifdef CLUEAPI_USE_LOGGING_MODULE
        /**
         * @brief Configuration for the logging module. See logging_cfg_t for details.
         */
        logging_cfg_t m_logging_cfg{};
#endif // CLUEAPI_USE_LOGGING_MODULE
    };
} // namespace clueapi::cfg

#endif // CLUEAPI_CFG_HXX