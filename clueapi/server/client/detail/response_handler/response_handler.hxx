/**
 * @file response_handler.hxx
 *
 * @brief Contains the implementation of the client-related functionality.
 */

#ifndef CLUEAPI_SERVER_CLIENT_DETAIL_RESPONSE_HANDLER_HXX
#define CLUEAPI_SERVER_CLIENT_DETAIL_RESPONSE_HANDLER_HXX

namespace clueapi::server {
    class c_server;

    namespace client::detail {
        /**
         * @brief Represents the response handler.
         *
         * @note This class is responsible for handling the response of a client.
         */
        struct response_handler_t {
            /**
             * @brief Constructs a new response handler.
             *
             * @param server The server instance.
             * @param socket The socket of the client.
             * @param cfg The configuration settings.
             * @param data The data of the client.
             */
            CLUEAPI_INLINE response_handler_t(
                server::c_server& server,
                boost::asio::ip::tcp::socket& socket,
                const cfg::cfg_t& cfg,
                data_t& data)
                : m_server{server}, m_socket{socket}, m_cfg{cfg}, m_data{data} {
            }

           public:
            /**
             * @brief Handles the response of the client.
             *
             * @return The expected result of the operation.
             */
            exceptions::expected_awaitable_t<void> handle();

            /**
             * @brief Sends an error response to the client.
             *
             * @param status_code The status code of the response.
             * @param error_message The error message of the response.
             */
            shared::awaitable_t<void> send_error_response(
                std::uint32_t status_code, std::string error_message);

           private:
            /**
             * @brief Handles the body response of the client.
             *
             * @return The expected result of the operation.
             */
            exceptions::expected_awaitable_t<void> raw_handle();

            /**
             * @brief Handles the stream response of the client.
             *
             * @return The expected result of the operation.
             */
            exceptions::expected_awaitable_t<void> stream_handle();

           private:
            /**
             * @brief Prepares the response of the client.
             *
             * @tparam _type_t The type of the response.
             *
             * @param response The response to prepare.
             * @param version The version of the response.
             *
             * @note This method is used to prepare the response of the client.
             */
            template <typename _type_t>
            void prepare_response(
                boost::beast::http::response<_type_t>& response, std::uint32_t version);

           private:
            /**
             * @brief The server instance.
             */
            server::c_server& m_server;

            /**
             * @brief The socket of the client.
             */
            boost::asio::ip::tcp::socket& m_socket;

            /**
             * @brief The configuration settings.
             */
            const cfg::cfg_t& m_cfg;

            /**
             * @brief The data of the client.
             */
            data_t& m_data;
        };
    } // namespace client::detail
} // namespace clueapi::server

#endif // CLUEAPI_SERVER_CLIENT_DETAIL_RESPONSE_HANDLER_HXX