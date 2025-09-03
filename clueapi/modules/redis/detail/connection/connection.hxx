#ifndef CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX
#define CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX

namespace clueapi::modules::redis::detail {
    using raw_connection_t = std::shared_ptr<boost::redis::connection>;

    struct connection_t {
        struct state_t {
            enum e_state { idle, connecting, connected, disconnected, error, unknown };

           public:
            CLUEAPI_INLINE state_t() = default;

            CLUEAPI_INLINE state_t(e_state state) : m_state{state} {
            }

           public:
            CLUEAPI_INLINE e_state get() const noexcept {
                return m_state.load(std::memory_order_acquire);
            }

            CLUEAPI_INLINE void set(e_state state) noexcept {
                m_state.store(state, std::memory_order_release);
            }

            CLUEAPI_INLINE bool compare_exchange_strong(
                e_state expected, e_state desired) noexcept {
                return m_state.compare_exchange_strong(
                    expected, desired, std::memory_order_acq_rel);
            }

           private:
            std::atomic<e_state> m_state{e_state::idle};
        };

        struct cfg_t {
            std::string m_host{"127.0.0.1"};

            std::string m_port{"6379"};

            std::string m_username{"default"};

            std::string m_password{};

            std::string m_client_name{"client-name"};

            std::string m_uuid{"client-uuid"};

            std::int32_t m_db{0};

            std::chrono::seconds m_connect_timeout{5};

            std::chrono::seconds m_health_check_interval{30};

            std::chrono::seconds m_reconnect_wait_interval{1};

            boost::redis::logger::level m_log_level{boost::redis::logger::level::info};

            bool m_use_ssl{false};
        };

       public:
        CLUEAPI_INLINE connection_t() = delete;

        CLUEAPI_INLINE connection_t(cfg_t cfg, boost::asio::io_context& io_ctx)
            : m_cfg{std::move(cfg)},
              m_io_ctx{io_ctx},
              m_state{state_t::idle},
              m_connection{std::make_shared<boost::redis::connection>(io_ctx)} {
        }

        CLUEAPI_INLINE ~connection_t() {
            disconnect();
        }

       public:
        shared::awaitable_t<bool> connect();

        shared::awaitable_t<bool> check_alive();

       public:
        void disconnect();

       public:
        template <typename... _tuple_t>
        CLUEAPI_NOINLINE shared::awaitable_t<boost::system::error_code> async_exec(
            const boost::redis::request& req, boost::redis::response<_tuple_t...>& resp) {
            boost::system::error_code ec{};

            co_await m_connection->async_exec(
                req, resp, boost::asio::redirect_error(boost::asio::use_awaitable, ec));

            co_return ec;
        }

        template <typename _type_t>
        CLUEAPI_NOINLINE shared::awaitable_t<std::optional<_type_t>> get(
            const std::string_view& key) {
            boost::redis::request req{};

            req.push("GET", key);

            boost::redis::response<std::optional<_type_t>> resp{};

            auto ec = co_await async_exec(req, resp);

            if (ec)
                co_return std::nullopt;

            co_return std::get<0>(resp).value();
        }

       public:
        shared::awaitable_t<bool> set(
            const std::string_view& key,
            const std::string_view& value,

            std::chrono::seconds ttl = std::chrono::seconds{0});

        shared::awaitable_t<bool> del(const std::string_view& key);

        shared::awaitable_t<bool> exists(const std::string_view& key);

        shared::awaitable_t<bool> expire(const std::string_view& key, std::chrono::seconds ttl);

        shared::awaitable_t<std::int32_t> ttl(const std::string_view& key);

        shared::awaitable_t<std::int32_t> lpush(
            const std::string_view& key, const std::string_view& value);

        shared::awaitable_t<bool> ltrim(
            const std::string_view& key, std::int32_t start, std::int32_t end);

        shared::awaitable_t<std::vector<std::string>> lrange(
            const std::string_view& key, std::int32_t start, std::int32_t end);

        shared::awaitable_t<std::int32_t> hset(
            const std::string_view& key,
            const std::unordered_map<std::string_view, std::string_view>& mapping);

        shared::awaitable_t<std::int32_t> hdel(
            const std::string_view& key, const std::vector<std::string_view>& fields);

        shared::awaitable_t<std::int32_t> hsetfield(
            const std::string_view& key,
            const std::string_view& field,
            const std::string_view& value);

        shared::awaitable_t<std::unordered_map<std::string, std::string>> hgetall(
            const std::string_view& key);

        shared::awaitable_t<std::int32_t> hincrby(
            const std::string_view& key, const std::string_view& field, std::int32_t increment);

        shared::awaitable_t<std::optional<std::string>> hget(
            const std::string_view& key, const std::string_view& field);

        shared::awaitable_t<bool> hexists(
            const std::string_view& key, const std::string_view& field);

        shared::awaitable_t<std::optional<std::int32_t>> incr(const std::string_view& key);
        
        shared::awaitable_t<std::optional<std::int32_t>> decr(const std::string_view& key);

       public:
        CLUEAPI_INLINE bool is_alive() const noexcept {
            return m_state.get() == state_t::connected;
        }

        CLUEAPI_INLINE state_t::e_state state() const noexcept {
            return m_state.get();
        }

        CLUEAPI_INLINE raw_connection_t raw_connection() const noexcept {
            return m_connection;
        }

       private:
        void build_raw_cfg();

       private:
        cfg_t m_cfg;

        state_t m_state;

        raw_connection_t m_connection;

        boost::redis::config m_raw_cfg;

        boost::asio::io_context& m_io_ctx;
    };
} // namespace clueapi::modules::redis::detail

#endif // CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX