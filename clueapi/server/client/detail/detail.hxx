/**
 * @file detail.hxx
 *
 * @brief Contains the implementation of the client-related functionality.
 */

#ifndef CLUEAPI_SERVER_CLIENT_DETAIL_DETAIL_HXX
#define CLUEAPI_SERVER_CLIENT_DETAIL_DETAIL_HXX

/**
 * @namespace clueapi::server::client::detail
 *
 * @brief Contains the implementation of the client-related functionality.
 */
namespace clueapi::server::client::detail {
    /**
     * @brief Closes a connection with an error code.
     *
     * @param ec The error code to close the connection with.
     * @param native_handle The native handle of the connection.
     * @param skip_left If true, skips logging the error code.
     *
     * @return True if the connection was closed, false otherwise.
     */
    CLUEAPI_INLINE bool close_connection(
        const boost::system::error_code& ec, std::int32_t native_handle, bool skip_left = false) {
        if (ec == boost::asio::error::operation_aborted) {
            CLUEAPI_LOG_TRACE("Socket operation cancelled (id: {})", native_handle);

            return true;
        }

        if (ec == boost::beast::http::error::end_of_stream) {
            CLUEAPI_LOG_TRACE("Client connection closed gracefully (id: {})", native_handle);

            return true;
        }

        if (ec == boost::asio::error::connection_reset) {
            CLUEAPI_LOG_TRACE("Client connection was reset (id: {})", native_handle);

            return true;
        }

        if (ec == boost::asio::error::eof) {
            CLUEAPI_LOG_TRACE("End of file reached (id: {})", native_handle);

            return true;
        }

        if (ec && !skip_left) {
            CLUEAPI_LOG_TRACE(
                "Connection operation error (id: {}): {}",

                native_handle,
                ec.message());

            return true;
        }

        return false;
    }

    /**
     * @brief Waits for a timer to expire.
     *
     * @param timer The timer to wait for.
     *
     * @return True if the timer expired, false otherwise.
     */
    static inline shared::awaitable_t<bool> wait_for_timer(boost::asio::steady_timer& timer) {
        boost::system::error_code ec{};

        co_await timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));

        if (ec == boost::asio::error::operation_aborted)
            co_return false;

        co_return true;
    }

    /**
     * @brief Executes an operation with a timeout.
     *
     * @tparam _op_t The operation type.
     *
     * @param op The operation to execute.
     * @param timer The timer to use for the timeout.
     *
     * @return The result of the operation.
     */
    template <typename _op_t>
    static inline exceptions::expected_awaitable_t<typename _op_t::value_type> exec_with_timeout(
        _op_t op, boost::asio::steady_timer& timer) {
        using namespace boost::asio::experimental::awaitable_operators;

        auto result = co_await (std::move(op) || wait_for_timer(timer));

        if (result.index() == 1)
            co_return exceptions::make_unexpected(
                exceptions::io_error_t::make("Operation timed out"));

        co_return exceptions::expected_t<typename _op_t::value_type>{std::get<0>(result)};
    }
} // namespace clueapi::server::client::detail

#include "data/data.hxx"

#include "request_handler/request_handler.hxx"

#include "response_handler/response_handler.hxx"

#endif // CLUEAPI_SERVER_CLIENT_DETAIL_DETAIL_HXX