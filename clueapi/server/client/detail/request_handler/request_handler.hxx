/**
 * @file request_handler.hxx
 *
 * @brief Contains the implementation of the client-related functionality.
 */

#ifndef CLUEAPI_SERVER_CLIENT_DETAIL_REQUEST_HANDLER_HXX
#define CLUEAPI_SERVER_CLIENT_DETAIL_REQUEST_HANDLER_HXX

namespace clueapi::server::client::detail {
    /**
     * @brief The error codes for the request handler.
     *
     * @note These error codes are used to return the appropriate HTTP status code.
     */
    enum struct e_error_code : std::uint16_t {
        success = 200,
        bad_request = 400,
        timeout = 408,
        payload_too_large = 413,
        internal_server_error = 500
    };

    /**
     * @struct req_handler_t
     *
     * @brief Handles the request of a client.
     *
     * @note This class is responsible for handling the request of a client.
     */
    struct req_handler_t {
        /**
         * @brief Constructs a new request handler.
         *
         * @param socket The socket of the client.
         * @param cfg The configuration settings.
         * @param data The data of the client.
         */
        CLUEAPI_INLINE req_handler_t(
            boost::asio::ip::tcp::socket& socket, const cfg::cfg_t& cfg, data_t& data)
            : m_socket{socket}, m_cfg{cfg}, m_data{data} {
        }

       public:
        /**
         * @brief Handles the request of the client.
         *
         * @return The error code of the request.
         */
        exceptions::expected_awaitable_t<e_error_code> handle();

       private:
        /**
         * @brief Handles the body request of the client.
         *
         * @return The error code of the request.
         */
        exceptions::expected_awaitable_t<e_error_code> raw_handle(
            boost::beast::http::request_parser<boost::beast::http::empty_body>&& hdr_parser);

        /**
         * @brief Handles the stream request of the client.
         *
         * @return The error code of the request.
         */
        exceptions::expected_awaitable_t<e_error_code> stream_handle(
            boost::beast::http::request_parser<boost::beast::http::empty_body>&& hdr_parser,
            boost::filesystem::path path,
            std::size_t content_length);

       private:
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
} // namespace clueapi::server::client::detail

#endif // CLUEAPI_SERVER_CLIENT_DETAIL_REQUEST_HANDLER_HXX