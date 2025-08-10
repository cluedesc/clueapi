/**
 * @file client.hxx
 *
 * @brief Defines the client class that handles a single connection.
 */

#ifndef CLUEAPI_SERVER_CLIENT_HXX
#define CLUEAPI_SERVER_CLIENT_HXX

namespace clueapi {
    class c_clueapi;

    namespace server {
        class c_server;

        /**
         * @struct client_t
         *
         * @brief Manages a single client connection to the server.
         *
         * @details This class encapsulates the logic for handling a single client:
         * reading requests, parsing them, passing them to the application for handling,
         * and sending back responses. Each instance is intended to be reused for
         * multiple requests over a keep-alive connection.
         *
         * @internal
         */
        struct client_t {
            /**
             * @brief Constructs a new client handler.
             *
             * @param clueapi A reference to the main clueapi application instance.
             * @param server A reference to the server that owns this client.
             */
            client_t(c_clueapi& clueapi, c_server& server);

            /**
             * @brief Resets the client's state to handle a new socket connection.
             *
             * @param socket The new TCP socket to handle.
             */
            void reset(boost::asio::ip::tcp::socket&& socket);

          public:
            /**
             * @brief Starts the request-response loop for this client.
             */
            shared::awaitable_t<void> start();

            /**
             * @brief Passes a parsed request to the main application logic and sends the response.
             *
             * @param req The parsed HTTP request from the client.
             *
             * @return An awaitable that resolves to `true` on success, `false` on failure.
             */
            shared::awaitable_t<bool> handle_request(http::types::request_t& req);

            /**
             * @brief Sends a pre-formatted error response to the client.
             *
             * @param status_code The HTTP status code of the error.
             * @param error_message A descriptive message for the error.
             */
            shared::awaitable_t<void> send_error_response(std::size_t status_code, std::string error_message);

          private:
            /**
             * @brief Prepares a Beast response object with common headers and settings.
             *
             * @tparam _type_t The body type of the response.
             *
             * @param response The response object to prepare.
             * @param keep_alive Whether to set keep-alive headers.
             */
            template <typename _type_t>
            void prepare_response(boost::beast::http::response<_type_t>& response, bool keep_alive, std::uint32_t version);

          private:
            /**
             * @brief Whether the client is closed.
             */
            bool m_close{};

            /**
             * @brief A reference to the clueapi.
             */
            c_clueapi& m_clueapi;

            /**
             * @brief A reference to the server.
             */
            c_server& m_server;

            /**
             * @brief The socket to the client.
             */
            std::optional<boost::asio::ip::tcp::socket> m_socket;

            /**
             * @brief The buffer for the client's request.
             */
            boost::beast::flat_buffer m_buffer;

            /**
             * @brief The parsed request.
             */
            http::types::request_t m_request;

            /**
             * @brief The parsed response.
             */
            http::types::response_t m_response_data;

            /**
             * @brief The parsed request body.
             */
            boost::beast::http::message<true, boost::beast::http::string_body> m_parsed_request;
        };
    }
}

#endif // CLUEAPI_SERVER_CLIENT_HXX