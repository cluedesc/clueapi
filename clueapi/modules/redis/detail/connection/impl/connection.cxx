#include <clueapi.hxx>

namespace clueapi::modules::redis::detail {
    shared::awaitable_t<bool> connection_t::async_connect() {
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

            auto timer = boost::asio::steady_timer{*m_io_ctx};

            timer.expires_after(k_check_interval);

            co_await timer.async_wait(boost::asio::use_awaitable);
        }

        m_state.set(state_t::error);

        cancel_connection();

        co_return false;
    }

    shared::awaitable_t<bool> connection_t::async_check_alive() {
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

    bool connection_t::sync_connect() {
        auto expected = state_t::idle;

        if (!m_state.compare_exchange_strong(expected, state_t::connecting)) {
            if (expected == state_t::connected)
                return true;

            return false;
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

        std::uint32_t iteration_count{};

        constexpr auto k_max_iteration_count = 250;

        while (iteration_count < k_max_iteration_count) {
            boost::redis::request::config req_cfg{
                .cancel_on_connection_lost = true,
                .cancel_if_not_connected = true,
                .cancel_if_unresponded = true};

            boost::redis::request req{req_cfg};

            req.push("PING", "PONG");

            boost::redis::response<std::string> resp{};

            auto ec = exec(req, resp);

            if (!ec && std::get<0>(resp).value() == "PONG") {
                m_state.set(state_t::connected);

                return true;
            }

            auto current_state = m_state.get();

            if (current_state == state_t::error || current_state == state_t::disconnected) {
                cancel_connection();

                return false;
            }
        }

        m_state.set(state_t::error);

        cancel_connection();

        return false;
    }

    bool connection_t::sync_check_alive() {
        auto current_state = m_state.get();

        if (current_state == state_t::error || current_state == state_t::disconnected ||
            current_state == state_t::idle)
            return false;

        boost::redis::request::config req_cfg{
            .cancel_on_connection_lost = true,
            .cancel_if_not_connected = true,
            .cancel_if_unresponded = true};

        boost::redis::request req{req_cfg};

        req.push("PING", "PONG");

        boost::redis::response<std::string> resp{};

        auto ec = exec(req, resp);

        if (!ec && std::get<0>(resp).value() == "PONG") {
            m_state.set(state_t::connected);

            return true;
        }

        m_state.set(state_t::error);

        return false;
    }

    shared::awaitable_t<bool> connection_t::async_set(
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

    shared::awaitable_t<bool> connection_t::async_del(const std::string_view& key) {
        boost::redis::request req{};

        req.push("DEL", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() > 0;
    }

    shared::awaitable_t<bool> connection_t::async_exists(const std::string_view& key) {
        boost::redis::request req{};

        req.push("EXISTS", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() > 0;
    }

    shared::awaitable_t<bool> connection_t::async_expire(
        const std::string_view& key, std::chrono::seconds ttl) {
        boost::redis::request req{};

        req.push("EXPIRE", key, std::to_string(ttl.count()));

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() == 1;
    }

    shared::awaitable_t<std::int32_t> connection_t::async_ttl(const std::string_view& key) {
        boost::redis::request req{};

        req.push("TTL", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::async_lpush(
        const std::string_view& key, const std::string_view& value) {
        boost::redis::request req{};

        req.push("LPUSH", key, value);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<bool> connection_t::async_ltrim(
        const std::string_view& key, std::int32_t start, std::int32_t end) {
        boost::redis::request req{};

        req.push("LTRIM", key, std::to_string(start), std::to_string(end));

        boost::redis::response<std::string> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() == "OK";
    }

    shared::awaitable_t<std::vector<std::string>> connection_t::async_lrange(
        const std::string_view& key, std::int32_t start, std::int32_t end) {
        boost::redis::request req{};

        req.push("LRANGE", key, std::to_string(start), std::to_string(end));

        boost::redis::response<std::vector<std::string>> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::vector<std::string>{};

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::async_hset(
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

    shared::awaitable_t<std::int32_t> connection_t::async_hdel(
        const std::string_view& key, const std::vector<std::string_view>& fields) {
        boost::redis::request req{};

        req.push_range("HDEL", key, fields);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::async_hsetfield(
        const std::string_view& key, const std::string_view& field, const std::string_view& value) {
        boost::redis::request req{};

        req.push("HSET", key, field, value);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::unordered_map<std::string, std::string>> connection_t::async_hgetall(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("HGETALL", key);

        boost::redis::response<std::unordered_map<std::string, std::string>> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::unordered_map<std::string, std::string>{};

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::int32_t> connection_t::async_hincrby(
        const std::string_view& key, const std::string_view& field, std::int32_t increment) {
        boost::redis::request req{};

        req.push("HINCRBY", key, field, std::to_string(increment));

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return 0;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::optional<std::string>> connection_t::async_hget(
        const std::string_view& key, const std::string_view& field) {
        boost::redis::request req{};

        req.push("HGET", key, field);

        boost::redis::response<std::optional<std::string>> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::nullopt;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<bool> connection_t::async_hexists(
        const std::string_view& key, const std::string_view& field) {
        boost::redis::request req{};

        req.push("HEXISTS", key, field);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return false;

        co_return std::get<0>(resp).value() == 1;
    }

    shared::awaitable_t<std::optional<std::int32_t>> connection_t::async_incr(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("INCR", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::nullopt;

        co_return std::get<0>(resp).value();
    }

    shared::awaitable_t<std::optional<std::int32_t>> connection_t::async_decr(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("DECR", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = co_await async_exec(req, resp);

        if (ec)
            co_return std::nullopt;

        co_return std::get<0>(resp).value();
    }

    // ...

    bool connection_t::sync_del(const std::string_view& key) {
        boost::redis::request req{};

        req.push("DEL", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return false;

        return std::get<0>(resp).value() > 0;
    }

    bool connection_t::sync_exists(const std::string_view& key) {
        boost::redis::request req{};

        req.push("EXISTS", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return false;

        return std::get<0>(resp).value() > 0;
    }

    bool connection_t::sync_expire(
        const std::string_view& key, std::chrono::seconds ttl) {
        boost::redis::request req{};

        req.push("EXPIRE", key, std::to_string(ttl.count()));

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return false;

        return std::get<0>(resp).value() == 1;
    }

    std::int32_t connection_t::sync_ttl(const std::string_view& key) {
        boost::redis::request req{};

        req.push("TTL", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return 0;

        return std::get<0>(resp).value();
    }

    std::int32_t connection_t::sync_lpush(
        const std::string_view& key, const std::string_view& value) {
        boost::redis::request req{};

        req.push("LPUSH", key, value);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return 0;

        return std::get<0>(resp).value();
    }

    bool connection_t::sync_ltrim(
        const std::string_view& key, std::int32_t start, std::int32_t end) {
        boost::redis::request req{};

        req.push("LTRIM", key, std::to_string(start), std::to_string(end));

        boost::redis::response<std::string> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return false;

        return std::get<0>(resp).value() == "OK";
    }

    std::vector<std::string> connection_t::sync_lrange(
        const std::string_view& key, std::int32_t start, std::int32_t end) {
        boost::redis::request req{};

        req.push("LRANGE", key, std::to_string(start), std::to_string(end));

        boost::redis::response<std::vector<std::string>> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return std::vector<std::string>{};

        return std::get<0>(resp).value();
    }

    std::int32_t connection_t::sync_hset(
        const std::string_view& key,
        const std::unordered_map<std::string_view, std::string_view>& mapping) {
        boost::redis::request req{};

        req.push_range("HSET", key, mapping);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return 0;

        return std::get<0>(resp).value();
    }

    std::int32_t connection_t::sync_hdel(
        const std::string_view& key, const std::vector<std::string_view>& fields) {
        boost::redis::request req{};

        req.push_range("HDEL", key, fields);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return 0;

        return std::get<0>(resp).value();
    }

    std::int32_t connection_t::sync_hsetfield(
        const std::string_view& key, const std::string_view& field, const std::string_view& value) {
        boost::redis::request req{};

        req.push("HSET", key, field, value);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return 0;

        return std::get<0>(resp).value();
    }

    std::unordered_map<std::string, std::string> connection_t::sync_hgetall(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("HGETALL", key);

        boost::redis::response<std::unordered_map<std::string, std::string>> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return std::unordered_map<std::string, std::string>{};

        return std::get<0>(resp).value();
    }

    std::int32_t connection_t::sync_hincrby(
        const std::string_view& key, const std::string_view& field, std::int32_t increment) {
        boost::redis::request req{};

        req.push("HINCRBY", key, field, std::to_string(increment));

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return 0;

        return std::get<0>(resp).value();
    }

    std::optional<std::string> connection_t::sync_hget(
        const std::string_view& key, const std::string_view& field) {
        boost::redis::request req{};

        req.push("HGET", key, field);

        boost::redis::response<std::optional<std::string>> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return std::nullopt;

        return std::get<0>(resp).value();
    }

    bool connection_t::sync_hexists(
        const std::string_view& key, const std::string_view& field) {
        boost::redis::request req{};

        req.push("HEXISTS", key, field);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return false;

        return std::get<0>(resp).value() == 1;
    }

    std::optional<std::int32_t> connection_t::sync_incr(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("INCR", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return std::nullopt;

        return std::get<0>(resp).value();
    }

    std::optional<std::int32_t> connection_t::sync_decr(
        const std::string_view& key) {
        boost::redis::request req{};

        req.push("DECR", key);

        boost::redis::response<std::int32_t> resp{};

        auto ec = exec(req, resp);

        if (ec)
            return std::nullopt;

        return std::get<0>(resp).value();
    }
} // namespace clueapi::modules::redis::detail