#include <boost/redis/src.hpp>

#include <clueapi.hxx>

namespace clueapi::modules::redis {
    void c_redis::init(cfg_t cfg, boost::asio::io_context* io_ctx) {
        if (m_running)
            return;

        m_cfg = std::move(cfg);

        m_io_ctx = io_ctx;

        m_running = true;
    }

    void c_redis::shutdown() {
        if (!m_running)
            return;

        m_running = false;
    }

    std::shared_ptr<connection_t> c_redis::create_connection(
        std::optional<connection_t::cfg_t> cfg, boost::asio::io_context* io_ctx) {
        if (!m_running || (!io_ctx && !m_io_ctx))
            return nullptr;

        auto final_cfg = connection_t::cfg_t{};

        if (!cfg.has_value()) {
            final_cfg.m_host = m_cfg.m_host;
            final_cfg.m_port = m_cfg.m_port;

            final_cfg.m_use_ssl = m_cfg.m_use_ssl;

            if (!m_cfg.m_password.empty())
                final_cfg.m_password = m_cfg.m_password;

            if (!m_cfg.m_username.empty())
                final_cfg.m_username = m_cfg.m_username;

            {
                final_cfg.m_client_name = "clueapi-redis-client";

                auto uuid = boost::uuids::to_string(boost::uuids::random_generator()());

                final_cfg.m_uuid = std::move(uuid);
            }

            {
                final_cfg.m_connect_timeout = m_cfg.m_connect_timeout;

                final_cfg.m_health_check_interval = m_cfg.m_health_check_interval;

                final_cfg.m_reconnect_wait_interval = m_cfg.m_reconnect_wait_interval;
            }

            final_cfg.m_db = m_cfg.m_db;

            final_cfg.m_log_level = m_cfg.m_log_level;
        } else
            final_cfg = std::move(*cfg);

        auto final_io_ctx = io_ctx ? io_ctx : m_io_ctx;

        return std::make_shared<connection_t>(std::move(final_cfg), *final_io_ctx);
    }
} // namespace clueapi::modules::redis