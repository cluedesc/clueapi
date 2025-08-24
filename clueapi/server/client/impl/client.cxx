/**
 * @file client.cxx
 *
 * @brief Implements the client class that handles a single connection.
 */

#include <clueapi.hxx>

namespace clueapi::server::client {
    client_t::client_t(c_server& server, const cfg::cfg_t& cfg)
        : m_server{server}, m_cfg{cfg}, m_data{cfg.m_server.m_client.m_buffer_capacity} {
    }

    client_t::~client_t() {
        m_data.force_cleanup();
    }

    shared::awaitable_t<void> client_t::start() {
        if (!m_server.is_running()) {
            CLUEAPI_LOG_TRACE("Server not running, aborting client execution");

            co_return;
        }

        if (!m_data.is_connected()) {
            CLUEAPI_LOG_WARNING("Client socket is invalid or closed");

            co_return;
        }

        auto& socket = m_data.m_socket.value();

        const auto& native_handle = socket.native_handle();

        CLUEAPI_LOG_TRACE("Starting client session (id: {})", native_handle);

        const auto has_keep_alive = m_cfg.m_http.m_keep_alive_enabled;
        const auto has_socket_timeout = m_cfg.m_socket.m_timeout.count() > 0;

        if (m_data.m_timeout_timer) {
            if (has_keep_alive) {
                m_data.m_timeout_timer->expires_after(m_cfg.m_http.m_keep_alive_timeout);
            } else if (has_socket_timeout)
                m_data.m_timeout_timer->expires_after(m_cfg.m_socket.m_timeout);
            else
                m_data.m_timeout_timer.reset();
        }

        try {
            while (m_server.is_running(std::memory_order_relaxed) && m_data.is_connected()) {
                detail::req_handler_t req_handler{socket, m_cfg, m_data};

                detail::response_handler_t response_handler{m_server, socket, m_cfg, m_data};

                {
                    auto result = co_await req_handler.handle();

                    if (!result.has_value())
                        break;

                    if (result.value() != detail::e_error_code::success) {
                        auto status = static_cast<http::types::status_t::e_status>(result.value());

                        co_await response_handler.send_error_response(
                            status, http::types::status_t::to_str_copy(status));

                        break;
                    }
                }

                {
                    auto result = co_await response_handler.handle();

                    {
                        m_data.m_request.reset();

                        m_data.m_beast_request.clear();
                    }

                    if (!result.has_value())
                        break;

                    // ...
                }

                CLUEAPI_LOG_TRACE("Successfully processed request (id: {})", native_handle);

                if (!has_keep_alive || m_data.m_should_close)
                    break;

                m_data.cut_buffer(m_cfg.m_server.m_client.m_buffer_capacity);

                if (m_data.m_timeout_timer && has_keep_alive)
                    m_data.m_timeout_timer->expires_after(m_cfg.m_http.m_keep_alive_timeout);
            }

            CLUEAPI_LOG_TRACE("Client session completed (id: {})", native_handle);
        } catch (const std::exception& e) {
            CLUEAPI_LOG_ERROR("Exception in client session (id: {}): {}", native_handle, e.what());
        } catch (...) {
            CLUEAPI_LOG_ERROR("Unknown exception in client session (id: {})", native_handle);
        }

        co_return;
    }
} // namespace clueapi::server::client