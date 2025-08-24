#include <clueapi.hxx>

namespace clueapi::server::client::detail {
    exceptions::expected_awaitable_t<void> response_handler_t::handle() {
        m_data.m_response_data.reset();

        {
            m_data.m_response_data.status() = http::types::status_t::unknown;

            if (m_server.middleware_chain()) {
                m_data.m_response_data = co_await m_server.middleware_chain()(m_data.m_request);
            } else
                CLUEAPI_LOG_ERROR("Middleware chain is not initialized");

            if (m_data.m_response_data.status() == http::types::status_t::unknown) {
                const auto status = http::types::status_t::internal_server_error;

                co_await send_error_response(status, http::types::status_t::to_str_copy(status));

                co_return exceptions::expected_t<void>{};
            }
        }

        if (m_data.m_response_data.is_stream())
            co_return co_await stream_handle();

        co_return co_await raw_handle();
    }

    exceptions::expected_awaitable_t<void> response_handler_t::raw_handle() {
        boost::beast::http::response<boost::beast::http::string_body> response{};

        const auto version = m_data.m_beast_request.version();

        prepare_response(response, version);

        {
            response.body() = std::move(m_data.m_response_data).move_body();

            response.result(m_data.m_response_data.status());
        }

        response.prepare_payload();

        boost::system::error_code ec{};

        if (m_data.m_timeout_timer) {
            auto expected = co_await exec_with_timeout(
                boost::beast::http::async_write(
                    m_socket,

                    response,

                    boost::asio::redirect_error(boost::asio::use_awaitable, ec)),

                *m_data.m_timeout_timer);

            if (!expected.has_value())
                co_return exceptions::make_unexpected("Operation timed out");
        } else {
            co_await boost::beast::http::async_write(
                m_socket,

                response,

                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }

        if (close_connection(ec, m_socket.native_handle()))
            co_return exceptions::make_unexpected("Connection closed");

        co_return exceptions::expected_t<void>{};
    }

    exceptions::expected_awaitable_t<void> response_handler_t::stream_handle() {
        boost::beast::http::response<boost::beast::http::empty_body> response{};

        const auto version = m_data.m_beast_request.version();

        prepare_response(response, version);

        {
            response.chunked(true);

            response.result(m_data.m_response_data.status());
        }

        boost::beast::http::serializer<false, boost::beast::http::empty_body> sr{response};

        boost::system::error_code ec{};

        if (m_data.m_timeout_timer) {
            auto expected = co_await exec_with_timeout(
                boost::beast::http::async_write_header(
                    m_socket, sr, boost::asio::redirect_error(boost::asio::use_awaitable, ec)),

                *m_data.m_timeout_timer);
        } else {
            co_await boost::beast::http::async_write_header(
                m_socket, sr, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }

        const auto& native_handle = m_socket.native_handle();

        if (ec) {
            CLUEAPI_LOG_ERROR(
                "Error writing response header (id: {}): {}", native_handle, ec.message());

            co_return exceptions::make_unexpected("Failed to write response header");
        }

        {
            http::chunks::chunk_writer_t writer{m_socket};

            auto stream_success = co_await m_data.m_response_data.stream_fn()(writer);

            if (!stream_success.has_value()) {
                CLUEAPI_LOG_ERROR(
                    "Error in response callback (id: {}): message={}",

                    native_handle,
                    stream_success.error());
            }

            if (!writer.final_chunk_written()) {
                auto chunk_written = co_await writer.write_final_chunk();

                if (!chunk_written.has_value()) {
                    CLUEAPI_LOG_ERROR(
                        "Error in response callback (id: {}): message={}",

                        native_handle,
                        chunk_written.error());
                }
            }
        }

        co_return exceptions::expected_t<void>{};
    }

    template <typename _type_t>
    void response_handler_t::prepare_response(
        boost::beast::http::response<_type_t>& response, std::uint32_t version) {
        response.version(version);

        response.result(static_cast<boost::beast::http::status>(m_data.m_response_data.status()));

        for (const auto& [key, value] : m_data.m_response_data.headers())
            response.insert(key, value);

        for (const auto& cookie : m_data.m_response_data.cookies())
            response.insert("Set-Cookie", cookie);

        const auto keep_alive = m_data.m_request.keep_alive();

        if (keep_alive) {
            response.keep_alive(true);

            response.set(boost::beast::http::field::connection, "keep-alive");
            response.set(boost::beast::http::field::keep_alive, m_server.get_keep_alive_timeout());

            m_data.m_should_close = false;
        } else {
            response.keep_alive(false);
            response.set(boost::beast::http::field::connection, "close");

            m_data.m_should_close = true;
        }
    }

    shared::awaitable_t<void> response_handler_t::send_error_response(
        std::uint32_t status_code, std::string error_message) {
        boost::beast::http::response<boost::beast::http::string_body> response{};

        response.version(m_data.m_beast_request.version());

        response.result(static_cast<boost::beast::http::status>(status_code));

        if (m_cfg.m_http.m_def_response_class == http::types::e_response_class::plain) {
            response.body() = http::types::status_t::to_str(status_code);
        } else {
            response.body() = http::types::json_t::serialize(http::types::json_t::json_obj_t{
                {"error", http::types::status_t::to_str(status_code)},
                {"detail", std::move(error_message)}});
        }

        response.prepare_payload();

        boost::system::error_code ec{};

        if (m_data.m_timeout_timer) {
            auto expected = co_await exec_with_timeout(
                boost::beast::http::async_write(
                    m_socket,

                    response,

                    boost::asio::redirect_error(boost::asio::use_awaitable, ec)),

                *m_data.m_timeout_timer);
        } else {
            co_await boost::beast::http::async_write(
                m_socket,

                response,

                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }

        if (ec) {
            CLUEAPI_LOG_ERROR(
                "Error sending error response (id: {}): {}",

                m_socket.native_handle(),
                ec.message());
        }

        co_return;
    }
} // namespace clueapi::server::client::detail