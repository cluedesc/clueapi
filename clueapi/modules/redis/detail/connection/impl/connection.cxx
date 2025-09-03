#include <clueapi.hxx>

namespace clueapi::modules::redis::detail {
    shared::awaitable_t<bool> connection_t::connect() {
        auto expected = state_t::idle;

        if (!m_state.compare_exchange_strong(expected, state_t::connecting)) {
            if (expected == state_t::connected)
                co_return true;

            co_return false;
        }

        build_raw_cfg();

        m_connection->async_run(
            m_raw_cfg,

            {m_cfg.m_log_level},

            [&](const boost::system::error_code& ec) {
                if (ec)
                    m_state.set(state_t::error);

                if (m_connection)
                    m_connection->cancel();
            });

        auto start_time = std::chrono::steady_clock::now();

        constexpr auto k_check_interval = std::chrono::milliseconds{100};

        while (std::chrono::steady_clock::now() - start_time < m_cfg.m_connect_timeout) {
            boost::redis::request::config req_cfg{
                .cancel_on_connection_lost = true,
                .cancel_if_not_connected = true,
                .cancel_if_unresponded = true};

            boost::redis::request req{req_cfg};

            req.push("PING", "PONG");

            boost::redis::response<std::string> resp{};

            boost::system::error_code ec{};

            co_await m_connection->async_exec(
                std::move(req), resp, boost::asio::redirect_error(boost::asio::use_awaitable, ec));

            if (!ec && std::get<0>(resp).value() == "PONG") {
                m_state.set(state_t::connected);

                co_return true;
            }

            auto current_state = m_state.get();

            if (current_state == state_t::error || current_state == state_t::disconnected)
                co_return false;

            auto timer = boost::asio::steady_timer{m_io_ctx};

            timer.expires_after(k_check_interval);

            co_await timer.async_wait(boost::asio::use_awaitable);
        }

        m_state.set(state_t::error);

        if (m_connection)
            m_connection->cancel();

        co_return false;
    }

    void connection_t::disconnect() {
        auto current = m_state.get();

        if (current == state_t::disconnected || current == state_t::idle)
            return;

        if (m_connection)
            m_connection->cancel();

        m_state.set(state_t::disconnected);
    }

    shared::awaitable_t<bool> connection_t::check_alive() {
        auto current_state = m_state.get();

        if (current_state == state_t::error || current_state == state_t::disconnected ||
            current_state == state_t::idle)
            co_return false;

        boost::redis::request::config req_cfg{
            .cancel_on_connection_lost = true,
            .cancel_if_not_connected = true,
            .cancel_if_unresponded = true};

        boost::redis::request req{req_cfg};

        req.push("PING", "PONG");

        boost::redis::response<std::string> resp{};

        boost::system::error_code ec{};

        co_await m_connection->async_exec(
            std::move(req), resp, boost::asio::redirect_error(boost::asio::use_awaitable, ec));

        if (!ec && std::get<0>(resp).value() == "PONG") {
            m_state.set(state_t::connected);

            co_return true;
        }

        m_state.set(state_t::error);

        co_return false;
    }

    void connection_t::build_raw_cfg() {
        boost::redis::config cfg{};

        {
            cfg.addr.host = m_cfg.m_host;
            cfg.addr.port = m_cfg.m_port;

            if (!m_cfg.m_password.empty())
                cfg.password = m_cfg.m_password;

            if (!m_cfg.m_username.empty())
                cfg.username = m_cfg.m_username;

            if (m_cfg.m_db >= 0)
                cfg.database_index.emplace(m_cfg.m_db);
            else
                cfg.database_index.reset();

            cfg.clientname = m_cfg.m_client_name;

            if (!m_cfg.m_uuid.empty()) {
                cfg.log_prefix =
                    fmt::format(fmt::runtime("[{}.{}] "), m_cfg.m_client_name, m_cfg.m_uuid);
            } else
                cfg.log_prefix = fmt::format(fmt::runtime("[{}.{}] "), m_cfg.m_client_name);

            cfg.connect_timeout = m_cfg.m_connect_timeout;
            cfg.health_check_interval = m_cfg.m_health_check_interval;
            cfg.reconnect_wait_interval = m_cfg.m_reconnect_wait_interval;

            cfg.health_check_id = m_cfg.m_client_name;

            cfg.use_ssl = m_cfg.m_use_ssl;
        }

        m_raw_cfg = std::move(cfg);
    }
} // namespace clueapi::modules::redis::detail