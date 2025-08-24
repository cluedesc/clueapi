#include <clueapi.hxx>

namespace clueapi::server::client::detail {
    exceptions::expected_awaitable_t<e_error_code> req_handler_t::handle() {
        boost::beast::http::request_parser<boost::beast::http::empty_body>&& hdr_parser{};

        {
            hdr_parser.body_limit(m_cfg.m_http.m_max_request_size);

            hdr_parser.header_limit(m_cfg.m_http.m_max_hdrs_request_size);
        }

        boost::system::error_code ec{};

        if (m_data.m_timeout_timer) {
            auto expected = co_await exec_with_timeout(
                boost::beast::http::async_read_header(
                    m_socket,

                    m_data.m_buffer,

                    hdr_parser,

                    boost::asio::redirect_error(boost::asio::use_awaitable, ec)),

                *m_data.m_timeout_timer);

            if (!expected.has_value())
                co_return exceptions::make_unexpected("Operation timed out");
        } else {
            co_await boost::beast::http::async_read_header(
                m_socket,

                m_data.m_buffer,

                hdr_parser,

                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }

        const auto& native_handle = m_socket.native_handle();

        if (close_connection(ec, native_handle))
            co_return exceptions::make_unexpected("Connection closed");

        const auto& hdr_data = hdr_parser.get();

        if (boost::beast::websocket::is_upgrade(hdr_data)) {
            CLUEAPI_LOG_WARNING(
                "WebSocket upgrade requested (id: {}): {}",

                native_handle,
                hdr_data.target());

            co_return exceptions::make_unexpected("WebSocket upgrade requested");
        }

        {
            m_data.m_request.uri() = hdr_data.target();

            m_data.m_request.method() = http::types::method_t::from_str(hdr_data.method_string());

            for (const auto& field : hdr_data)
                m_data.m_request.headers().emplace(field.name_string(), field.value());
        }

        CLUEAPI_LOG_DEBUG(
            "Handle request (id: {}): uri: {}, method: {}",

            native_handle,
            m_data.m_request.uri(),
            http::types::method_t::to_str(m_data.m_request.method()));

        auto& hdrs = m_data.m_request.headers();

        if (auto content_type_it = hdrs.find("Content-Type"); content_type_it != hdrs.end()) {
            auto content_type = content_type_it->second;

            if (boost::algorithm::istarts_with(content_type, "multipart/form-data")) {
                auto boundary = shared::non_copy::extract_str(content_type, "boundary");

                if (boundary.empty())
                    co_return exceptions::expected_t<e_error_code>{e_error_code::bad_request};

                auto content_length_it = hdrs.find("Content-Length");

                if (content_length_it == hdrs.end())
                    co_return exceptions::expected_t<e_error_code>{e_error_code::bad_request};

                const auto content_length = std::stoull(content_length_it->second);

                if (content_length > m_cfg.m_http.m_max_request_size)
                    co_return exceptions::expected_t<e_error_code>{e_error_code::payload_too_large};

                auto tmp_file =
                    m_cfg.m_server.m_tmp_dir / boost::filesystem::unique_path("tmp-%%%%-%%%%");

                co_return co_await stream_handle(
                    std::move(hdr_parser), std::move(tmp_file), content_length);
            }
        }

        co_return co_await raw_handle(std::move(hdr_parser));
    }

    exceptions::expected_awaitable_t<e_error_code> req_handler_t::stream_handle(
        boost::beast::http::request_parser<boost::beast::http::empty_body>&& hdr_parser,
        boost::filesystem::path path,
        std::size_t content_length) {
        const auto& executor = co_await boost::asio::this_coro::executor;

        boost::asio::stream_file file(executor);

        try {
            boost::system::error_code ec{};

            const auto flags =
                boost::asio::stream_file::write_only | boost::asio::stream_file::create;

            file.open(path.string(), flags, ec);

            if (ec) {
                CLUEAPI_LOG_ERROR("Failed to open file for streaming: {}", ec.message());

                co_return exceptions::expected_t<e_error_code>{e_error_code::internal_server_error};
            }

            auto remaining = content_length;

            while (remaining > 0) {
                if (m_data.m_buffer.size()) {
                    auto sequence = m_data.m_buffer.data();

                    auto to_write = std::min(boost::asio::buffer_size(sequence), remaining);

                    auto written = co_await boost::asio::async_write(
                        file, boost::asio::buffer(sequence, to_write), boost::asio::use_awaitable);

                    if (written != to_write) {
                        CLUEAPI_LOG_ERROR(
                            "Failed to write to file for streaming (id: {}): {}",

                            m_socket.native_handle(),
                            ec.message());

                        co_return exceptions::expected_t<e_error_code>{
                            e_error_code::internal_server_error};
                    }

                    m_data.m_buffer.consume(written);

                    remaining -= written;

                    continue;
                }

                std::size_t read{};

                if (m_data.m_timeout_timer) {
                    auto expected = co_await exec_with_timeout(
                        m_socket.async_read_some(
                            m_data.m_buffer.prepare(m_cfg.m_http.m_chunk_size),

                            boost::asio::redirect_error(boost::asio::use_awaitable, ec)),

                        *m_data.m_timeout_timer);

                    if (!expected.has_value())
                        co_return exceptions::make_unexpected("Operation timed out");

                    read = expected.value();
                } else {
                    read = co_await m_socket.async_read_some(
                        m_data.m_buffer.prepare(m_cfg.m_http.m_chunk_size),

                        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                }

                if (read == 0)
                    co_return exceptions::expected_t<e_error_code>{e_error_code::bad_request};

                m_data.m_buffer.commit(read);
            }
        } catch (const std::exception& e) {
            CLUEAPI_LOG_ERROR("Exception in stream handler: {}", e.what());

            file.close();

            co_return exceptions::expected_t<e_error_code>{e_error_code::internal_server_error};
        }

        file.close();

        m_data.m_request.parse_path() = std::move(path);

        co_return exceptions::expected_t<e_error_code>{e_error_code::success};
    }

    exceptions::expected_awaitable_t<e_error_code> req_handler_t::raw_handle(
        boost::beast::http::request_parser<boost::beast::http::empty_body>&& hdr_parser) {
        bool has_body{};

        std::size_t content_length{};

        const auto& hdr_req = hdr_parser.get();

        if (auto content_length_it = hdr_req.find(boost::beast::http::field::content_length);
            content_length_it != hdr_req.end()) {
            content_length = std::stoull(content_length_it->value());

            has_body = content_length > 0;
        }

        boost::beast::http::request_parser<boost::beast::http::string_body> parser{
            std::move(hdr_parser)};

        parser.body_limit(m_cfg.m_http.m_max_request_size);

        if (has_body) {
            boost::system::error_code ec{};

            if (m_data.m_timeout_timer) {
                auto expected = co_await exec_with_timeout(
                    boost::beast::http::async_read(
                        m_socket,

                        m_data.m_buffer,
                        parser,
                        boost::asio::redirect_error(boost::asio::use_awaitable, ec)),

                    *m_data.m_timeout_timer);

                if (!expected.has_value())
                    co_return exceptions::make_unexpected("Operation timed out");
            } else {
                co_await boost::beast::http::async_read(
                    m_socket,

                    m_data.m_buffer,
                    parser,
                    boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            }

            if (close_connection(ec, m_socket.native_handle()))
                co_return exceptions::make_unexpected("Connection closed");

            if (ec) {
                if (ec == boost::beast::http::error::body_limit) {
                    co_return exceptions::expected_t<e_error_code>{e_error_code::payload_too_large};
                } else if (ec == boost::system::errc::timed_out)
                    co_return exceptions::expected_t<e_error_code>{e_error_code::timeout};

                co_return exceptions::expected_t<e_error_code>{e_error_code::bad_request};
            }

            m_data.m_beast_request = parser.release();

            const auto& body = m_data.m_beast_request.body();

            if (content_length > 0 && body.size() != content_length)
                co_return exceptions::expected_t<e_error_code>{e_error_code::bad_request};

            m_data.m_request.body() = std::move(std::move(m_data.m_beast_request).body());
        }

        co_return exceptions::expected_t<e_error_code>{e_error_code::success};
    }
} // namespace clueapi::server::client::detail