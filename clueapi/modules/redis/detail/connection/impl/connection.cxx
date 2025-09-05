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

                cancel_connection();
            });

        auto start_time = std::chrono::steady_clock::now();

        constexpr auto k_check_interval = std::chrono::milliseconds{50};

        while (std::chrono::steady_clock::now() - start_time < m_cfg.m_connect_timeout) {
            boost::redis::request::config req_cfg{
                .cancel_on_connection_lost = true,
                .cancel_if_not_connected = true,
                .cancel_if_unresponded = true};

            boost::redis::request req{req_cfg};

            req.push("PING", "PONG");

            boost::redis::response<std::string> resp{};

            auto ec = co_await async_exec(req, resp);

            if (!ec && std::get<0>(resp).value() == "PONG") {
                m_state.set(state_t::connected);

                co_return true;
            }

            auto current_state = m_state.get();

            if (current_state == state_t::error || current_state == state_t::disconnected) {
                cancel_connection();
                
                co_return false;
            }

            auto timer = boost::asio::steady_timer{m_io_ctx};

            timer.expires_after(k_check_interval);

            co_await timer.async_wait(boost::asio::use_awaitable);
        }

        m_state.set(state_t::error);

        cancel_connection();

        co_return false;
    }

    void connection_t::disconnect() {
        auto current = m_state.get();

        if (current == state_t::disconnected || current == state_t::idle)
            return;

        cancel_connection();

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

        auto ec = co_await async_exec(req, resp);

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

    shared::awaitable_t<bool> connection_t::set(
        const std::string_view& key, const std::string_view& value, std::chrono::seconds ttl) {
        boost::redis::request req{};

        if (ttl.count() > 0) {
            req.push("SET", key, value, "EX", std::to_string(ttl.count()));
        } else
            req.push("SET", key, value);

        boost::redis::response<std::string> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() == "OK";
    }

    shared::awaitable_t<bool> connection_t::del(const std::string_view& key) {
        boost::redis::request req{};

        req.push("DEL", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() > 0;
    }

    shared::awaitable_t<bool> connection_t::exists(const std::string_view& key) {
        boost::redis::request req{};

        req.push("EXISTS", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() > 0;
    }

    shared::awaitable_t<bool> connection_t::expire(
        const std::string_view& key, std::chrono::seconds ttl) {
        boost::redis::request req{};

        req.push("EXPIRE", key, std::to_string(ttl.count()));

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() == 1;
    }

    shared::awaitable_t<std::int32_t> connection_t::ttl(const std::string_view& key) {
        boost::redis::request req{};

        req.push("TTL", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::lpush(
        const std::string_view& key, const std::string_view& value) {
        boost::redis::request req{};

        req.push("LPUSH", key, value);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<bool> connection_t::ltrim(
        const std::string_view& key, std::int32_t start, std::int32_t end) {
        boost::redis::request req{};

        req.push("LTRIM", key, std::to_string(start), std::to_string(end));

        boost::redis::response<std::string> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() == "OK";
    }

    shared::awaitable_t<std::vector<std::string>> connection_t::lrange(
        const std::string_view& key, std::int32_t start, std::int32_t end) {
        boost::redis::request req{};

        req.push("LRANGE", key, std::to_string(start), std::to_string(end));

        boost::redis::response<std::vector<std::string>> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::vector<std::string>{};

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::hset(
        const std::string_view& key,
        const std::unordered_map<std::string_view, std::string_view>& mapping) {
        boost::redis::request req{};

        req.push_range("HSET", key, mapping);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::hdel(
        const std::string_view& key, const std::vector<std::string_view>& fields) {
        boost::redis::request req{};

        req.push_range("HDEL", key, fields);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::hsetfield(
        const std::string_view& key, const std::string_view& field, const std::string_view& value) {
        boost::redis::request req{};

        req.push("HSET", key, field, value);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::unordered_map<std::string, std::string>> connection_t::hgetall(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("HGETALL", key);

        boost::redis::response<std::unordered_map<std::string, std::string>> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::unordered_map<std::string, std::string>{};

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::hincrby(
        const std::string_view& key, const std::string_view& field, std::int32_t increment) {
        boost::redis::request req{};

        req.push("HINCRBY", key, field, std::to_string(increment));

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::optional<std::string>> connection_t::hget(
        const std::string_view& key, const std::string_view& field) {
        boost::redis::request req{};

        req.push("HGET", key, field);

        boost::redis::response<std::optional<std::string>> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::nullopt;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<bool> connection_t::hexists(
        const std::string_view& key, const std::string_view& field) {
        boost::redis::request req{};

        req.push("HEXISTS", key, field);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() == 1;
    }

    shared::awaitable_t<std::optional<std::int32_t>> connection_t::incr(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("INCR", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::nullopt;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::optional<std::int32_t>> connection_t::decr(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("DECR", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::nullopt;

        co_return std::get<0>(resp).value();
    }

    void connection_t::cancel_connection() {
        if (m_connection) {
            if (!m_is_cancelled.exchange(true, std::memory_order_acq_rel)) {
                m_connection->cancel();
            }
        }
    }
} // namespace clueapi::modules::redis::detail