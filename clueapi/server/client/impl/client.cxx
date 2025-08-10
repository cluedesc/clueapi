/**
 * @file client.cxx
 *
 * @brief Implements the client class that handles a single connection.
 */

#include <clueapi.hxx>

namespace clueapi::server {
    client_t::client_t(c_clueapi& clueapi, c_server& server) : m_clueapi{clueapi}, m_server{server} {
        m_buffer.reserve(m_clueapi.cfg().m_http.m_def_client_buffer_capacity);
    }

    void client_t::reset(boost::asio::ip::tcp::socket&& socket) {
        m_socket.emplace(std::move(socket));

        m_close = false;

        m_buffer.clear();

        m_parsed_request.clear();

        m_request.reset();

        m_response_data.reset();
    }

    exceptions::expected_awaitable_t<void> stream_req(
        boost::asio::ip::tcp::socket& socket,

        boost::beast::flat_buffer& buffer,

        std::size_t length, std::size_t chunk_size,

        boost::filesystem::path& path
    ) {
        auto executor = co_await boost::asio::this_coro::executor;

        boost::asio::stream_file file(executor);

        boost::system::error_code ec{};

        file.open(
            path.string(),

            boost::asio::stream_file::write_only | boost::asio::stream_file::create,

            ec
        );

        if (ec) {
            co_return exceptions::make_unexpected(exceptions::io_error_t::make(
                "Can't open temp file: {}",

                ec.message()
            ));
        }

        auto remaining = length;

        try {
            while (remaining > 0u) {
                if (buffer.size() > 0u) {
                    auto sequence = buffer.data();

                    auto to_write = std::min(boost::asio::buffer_size(sequence), remaining);

                    auto bytes_written = co_await async_write(
                        file,

                        boost::asio::buffer(sequence, to_write),

                        boost::asio::use_awaitable
                    );

                    if (bytes_written != to_write) {
                        co_return exceptions::make_unexpected(exceptions::io_error_t::make("Incomplete write to file"));
                    }

                    buffer.consume(bytes_written);

                    remaining -= bytes_written;

                    continue;
                }

                auto bytes_read = co_await socket.async_read_some(
                    buffer.prepare(chunk_size),

                    boost::asio::use_awaitable
                );

                if (bytes_read == 0u) {
                    co_return exceptions::make_unexpected(exceptions::io_error_t::make("Connection closed unexpectedly")
                    );
                }

                buffer.commit(bytes_read);
            }
        }
        catch (const std::exception& e) {
            file.close();

            co_return exceptions::make_unexpected(exceptions::io_error_t::make(
                "Exception in while streaming request: {}",

                e.what()
            ));
        }

        file.close();

        co_return exceptions::expected_t<void>{};
    }

    shared::awaitable_t<void> client_t::start() {
        try {
            if (!m_server.is_running(std::memory_order_acquire))
                co_return;

            while (m_server.is_running(std::memory_order_relaxed) && !m_close) {
                boost::beast::http::request_parser<boost::beast::http::empty_body> hdr_parser{};

                const auto& clueapi_cfg = m_clueapi.cfg();

                {
                    hdr_parser.body_limit(clueapi_cfg.m_server.m_max_request_size);

                    hdr_parser.header_limit(clueapi_cfg.m_server.m_max_hdrs_request_size);
                }

                boost::system::error_code ec{};

                co_await boost::beast::http::async_read_header(
                    *m_socket, m_buffer,

                    hdr_parser,

                    boost::asio::redirect_error(boost::asio::use_awaitable, ec)
                );

                if (ec == boost::beast::http::error::end_of_stream || ec == boost::asio::error::connection_reset
                    || ec == boost::asio::error::operation_aborted)
                    break;

                if (ec) {
                    CLUEAPI_LOG_ERROR(
                        "Error while reading header (id: {}): {}",

                        m_socket->native_handle(), ec.message()
                    );

                    if (ec == boost::beast::http::error::body_limit || ec == boost::beast::http::error::header_limit) {
                        co_await send_error_response(413u, ec.message());

                        continue;
                    }
                    else if (ec == boost::beast::http::error::bad_method || ec == boost::beast::http::error::bad_value
                             || ec == boost::beast::http::error::bad_version) {
                        co_await send_error_response(400u, ec.message());

                        continue;
                    }

                    co_await send_error_response(500u, ec.message());

                    continue;
                }

                const auto& hdr_req = hdr_parser.get();

                const auto is_websocket = boost::beast::websocket::is_upgrade(hdr_req);

                if (is_websocket)
                    break;

                {
                    m_request.uri() = hdr_req.target();

                    m_request.method() = http::types::method_t::from_str(hdr_req.method_string());

                    for (auto& field : hdr_req)
                        m_request.headers().emplace(field.name_string(), field.value());
                }

                bool is_stream_parsing{};

                auto ctype_it = m_request.headers().find("content-type");

                if (ctype_it != m_request.headers().end()) {
                    auto ctype = ctype_it->second;

                    if (boost::algorithm::istarts_with(ctype, "multipart/form-data")) {
                        is_stream_parsing = true;

                        auto boundary = http::ctx_t::extract_boundary(ctype);

                        auto clen_it = m_request.headers().find("content-length");

                        if (clen_it == m_request.headers().end()) {
                            CLUEAPI_LOG_ERROR(
                                "Missing Content-Length for multipart (id: {})",

                                m_socket->native_handle()
                            );

                            co_await send_error_response(400u, "Missing Content-Length for multipart");

                            continue;
                        }

                        auto clen = std::stoull(clen_it->second);

                        if (clen > clueapi_cfg.m_server.m_max_request_size) {
                            CLUEAPI_LOG_ERROR(
                                "Payload too large (id: {})",

                                m_socket->native_handle()
                            );

                            co_await send_error_response(413u, "Payload too large");

                            continue;
                        }

                        ec.clear();

                        auto tmp_file
                            = clueapi_cfg.m_server.m_tmp_dir / boost::filesystem::unique_path("upload-%%%%-%%%%");

                        auto stream_result = co_await stream_req(
                            *m_socket,

                            m_buffer,

                            clen,

                            clueapi_cfg.m_server.m_chunk_size,

                            tmp_file
                        );

                        if (!stream_result.has_value()) {
                            CLUEAPI_LOG_ERROR(
                                "Failed to stream request: {}",

                                stream_result.error()
                            );

                            co_await send_error_response(
                                500u,

                                stream_result.error()
                            );

                            continue;
                        }

                        m_request.parse_path() = std::move(tmp_file);
                    }
                }

                if (!is_stream_parsing) {
                    bool has_body{};

                    std::size_t content_length{};

                    auto content_length_it = hdr_req.find(boost::beast::http::field::content_length);

                    if (content_length_it != hdr_req.end()) {
                        bool has_error{};

                        try {
                            content_length = std::stoull(std::string(content_length_it->value()));

                            has_body = (content_length > 0);
                        }
                        catch (const std::exception& e) {
                            CLUEAPI_LOG_ERROR(
                                "Invalid Content-Length header (id: {}): {}", m_socket->native_handle(),
                                content_length_it->value()
                            );

                            has_error = true;
                        }

                        if (has_error) {
                            co_await send_error_response(400u, "Invalid Content-Length header");

                            continue;
                        }
                    }

                    boost::beast::http::request_parser<boost::beast::http::string_body> parser{std::move(hdr_parser)};

                    parser.body_limit(clueapi_cfg.m_server.m_max_request_size);

                    if (has_body) {
                        co_await boost::beast::http::async_read(
                            *m_socket,

                            m_buffer,

                            parser,

                            boost::asio::redirect_error(boost::asio::use_awaitable, ec)
                        );

                        if (ec == boost::beast::http::error::end_of_stream || ec == boost::asio::error::connection_reset
                            || ec == boost::asio::error::operation_aborted)
                            break;

                        if (ec) {
                            CLUEAPI_LOG_ERROR(
                                "Error while reading body (id: {}): {}", m_socket->native_handle(), ec.message()
                            );

                            if (ec == boost::beast::http::error::body_limit) {
                                co_await send_error_response(413u, "Request entity too large");
                            }
                            else if (ec == boost::system::errc::timed_out) {
                                co_await send_error_response(408u, "Request timeout");
                            }
                            else
                                co_await send_error_response(400u, "Bad request: " + ec.message());

                            continue;
                        }
                    }

                    m_parsed_request = std::move(parser.release());

                    const auto& body = m_parsed_request.body();

                    if (content_length > 0 && body.size() != content_length) {
                        CLUEAPI_LOG_WARNING(
                            "Body size mismatch: expected {}, got {} (id: {})", content_length, body.size(),

                            m_socket->native_handle()
                        );
                    }

                    m_request.body() = std::move(std::move(m_parsed_request).body());
                }

                auto handled = co_await handle_request(m_request);

                {
                    m_request.reset();

                    m_parsed_request.clear();
                }

                if (!handled)
                    break;

                if (m_buffer.capacity() > clueapi_cfg.m_http.m_def_client_buffer_capacity * 2) {
                    boost::beast::flat_buffer tmp{};

                    std::swap(m_buffer, tmp);

                    m_buffer.reserve(clueapi_cfg.m_http.m_def_client_buffer_capacity);
                }

                if (m_close)
                    break;
            }
        }
        catch (const std::exception& e) {
            CLUEAPI_LOG_ERROR(
                "Exception in client loop: {}",

                e.what()
            );
        }
        catch (...) {
            CLUEAPI_LOG_ERROR("Unknown exception in client loop");
        }

        m_buffer.clear();

        m_request.reset();

        m_parsed_request.clear();

        if (m_socket.has_value() && m_socket->is_open()) {
            boost::system::error_code ec;

            m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

            m_socket->close(ec);

            m_socket.reset();
        }

        co_return;
    }

    template <typename _type_t>
    void client_t::prepare_response(
        boost::beast::http::response<_type_t>& response, bool keep_alive, std::uint32_t version
    ) {
        response.version(version);

        response.result(static_cast<boost::beast::http::status>(m_response_data.status()));

        for (const auto& [key, value] : m_response_data.headers())
            response.insert(key, value);

        for (const auto& cookie : m_response_data.cookies())
            response.insert("Set-Cookie", cookie);

        if (keep_alive) {
            response.keep_alive(true);

            response.set(boost::beast::http::field::connection, "keep-alive");
            response.set(boost::beast::http::field::keep_alive, m_server.get_keep_alive_timeout());

            m_close = false;
        }
        else {
            response.keep_alive(false);
            response.set(boost::beast::http::field::connection, "close");

            m_close = true;
        }
    }

    shared::awaitable_t<bool> client_t::handle_request(http::types::request_t& req) {
        auto keep_alive = req.keep_alive();

        auto http_version = m_parsed_request.version();

        boost::system::error_code ec{};

        m_response_data.reset();

        auto _ = co_await m_clueapi.handle_request(req, m_response_data);

        const auto& cfg = m_clueapi.cfg();

        if (m_response_data.is_stream()) {
            boost::beast::http::response<boost::beast::http::empty_body> response{};

            prepare_response(response, keep_alive, http_version);

            {
                response.chunked(true);

                response.result(static_cast<boost::beast::http::status>(m_response_data.status()));
            }

            {
                boost::beast::http::serializer<false, boost::beast::http::empty_body> sr{response};

                co_await boost::beast::http::async_write_header(
                    *m_socket,

                    sr,

                    boost::asio::redirect_error(boost::asio::use_awaitable, ec)
                );

                if (ec) {
                    CLUEAPI_LOG_ERROR(
                        "Error writing response header (id: {}): {}",

                        m_socket->native_handle(), ec.message()
                    );

                    co_return false;
                }
            }

            ec.clear();

            {
                http::chunks::chunk_writer_t writer{*m_socket};

                auto stream_success = co_await m_response_data.stream_fn()(writer);

                if (!stream_success.has_value()) {
                    CLUEAPI_LOG_ERROR(
                        "Error in response callback (id: {}): message={}",

                        m_socket->native_handle(), stream_success.error()
                    );
                }

                if (!writer.final_chunk_written()) {
                    auto chunk_written = co_await writer.write_final_chunk();

                    if (!chunk_written.has_value()) {
                        CLUEAPI_LOG_ERROR(
                            "Error in response callback (id: {}): message={}",

                            m_socket->native_handle(), chunk_written.error()
                        );
                    }
                }
            }

            if (m_close) {
                boost::system::error_code ignored_ec{};

                m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send, ignored_ec);
            }

            co_return true;
        }

        boost::beast::http::response<boost::beast::http::string_body> response{};

        prepare_response(response, keep_alive, http_version);

        {
            response.body() = std::move(m_response_data.move_body());

            response.result(static_cast<std::int32_t>(m_response_data.status()));
        }

        ec.clear();

        response.prepare_payload();

        co_await boost::beast::http::async_write(
            *m_socket,

            response,

            boost::asio::redirect_error(boost::asio::use_awaitable, ec)
        );

        if (ec) {
            CLUEAPI_LOG_ERROR(
                "Error writing response (id: {}): message={}",

                m_socket->native_handle(), ec.message()
            );

            co_return false;
        }

        co_return true;
    }

    shared::awaitable_t<void> client_t::send_error_response(std::size_t status_code, std::string error_message) {
        boost::beast::http::response<boost::beast::http::string_body> response{};

        response.version(m_parsed_request.version());

        const auto& cfg = m_clueapi.cfg();

        response.result(static_cast<boost::beast::http::status>(status_code));

        if (cfg.m_http.m_def_response_class == http::types::e_response_class::plain) {
            response.body() = http::types::status_t::to_str(status_code);
        }
        else {
            response.body() = http::types::json_t::serialize(http::types::json_t::json_obj_t{
                {"error", http::types::status_t::to_str(status_code)}, {"detail", error_message}
            });
        }

        response.prepare_payload();

        boost::system::error_code write_ec{};

        co_await boost::beast::http::async_write(
            *m_socket,

            response,

            boost::asio::redirect_error(boost::asio::use_awaitable, write_ec)
        );

        if (write_ec) {
            CLUEAPI_LOG_ERROR(
                "Error sending error response (id: {}): {}",

                m_socket->native_handle(), write_ec.message()
            );
        }

        co_return;
    }
}