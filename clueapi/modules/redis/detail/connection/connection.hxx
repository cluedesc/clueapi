#ifndef CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX
#define CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX

namespace clueapi::modules::redis::detail {
    using raw_connection_t = std::shared_ptr<boost::redis::connection>;

    struct connection_t {
        struct state_t {
            enum e_state {
                idle,
                connecting,
                connected,
                disconnected,
                error,
                unknown
            };

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