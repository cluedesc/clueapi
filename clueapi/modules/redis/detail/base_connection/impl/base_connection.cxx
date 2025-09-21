#include <clueapi.hxx>

namespace clueapi::modules::redis::detail {
    void base_connection_t::disconnect() {
        auto current = m_state.get();

        if (current == state_t::disconnected || current == state_t::idle)
            return;

        cancel_connection();

        m_state.set(state_t::disconnected);
    }

    void base_connection_t::build_raw_cfg() {
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

    void base_connection_t::cancel_connection() {
        if (m_connection) {
            if (!m_is_cancelled.exchange(true, std::memory_order_acq_rel)) {
                m_connection->cancel();
            }
        }
    }
}