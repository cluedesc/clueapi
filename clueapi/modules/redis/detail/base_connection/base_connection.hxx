/**
 * @file base_connection.hxx
 *
 * @brief Defines the base class for Redis connection wrappers.
 *
 * @details This file provides the core components for managing a Redis connection,
 * including thread-safe state management, configuration, and a wrapper around the
 * underlying `boost::redis::connection`. It serves as the foundation for both
 * synchronous and asynchronous connection types.
 *
 * @internal
 */

#ifndef CLUEAPI_MODULES_REDIS_DETAIL_BASE_CONNECTION_HXX
#define CLUEAPI_MODULES_REDIS_DETAIL_BASE_CONNECTION_HXX

namespace clueapi::modules::redis::detail {
    /**
     * @brief The raw Redis connection type.
     */
    using raw_connection_t = boost::redis::connection;

    /**
     * @struct base_connection_t
     *
     * @brief A base class that encapsulates a Redis connection.
     *
     * @details Manages the lifecycle, configuration, and state of a single connection
     * to a Redis server. This class is move-only to ensure clear ownership of the
     * underlying connection resources.
     *
     * @internal
     */
    struct base_connection_t {
        /**
         * @struct state_t
         *
         * @brief Manages the connection state in a thread-safe manner.
         *
         * @details Provides atomic operations for connection state management,
         * ensuring thread-safe access to the current connection status.
         *
         * @internal
         */
        struct state_t {
            /**
             * @enum e_state
             *
             * @brief Defines the possible connection states.
             *
             * @internal
             */
            enum e_state { idle, connecting, connected, disconnected, error, unknown };

           public:
            CLUEAPI_INLINE state_t() = default;

            /**
             * @brief Constructs a state with the given initial state.
             *
             * @param state The initial connection state.
             */
            CLUEAPI_INLINE state_t(e_state state) : m_state{state} {
            }

           public:
            /**
             * @brief Gets the current connection state.
             *
             * @return The current connection state.
             */
            CLUEAPI_INLINE e_state get() const noexcept {
                return m_state.load(std::memory_order_acquire);
            }

            /**
             * @brief Sets the connection state.
             *
             * @param state The new connection state.
             */
            CLUEAPI_INLINE void set(e_state state) noexcept {
                m_state.store(state, std::memory_order_release);
            }

            /**
             * @brief Atomically compares and exchanges the connection state.
             *
             * @param expected The expected current state.
             * @param desired The desired new state.
             *
             * @return `true` if the exchange was successful, `false` otherwise.
             */
            CLUEAPI_INLINE bool compare_exchange_strong(
                e_state expected, e_state desired) noexcept {
                return m_state.compare_exchange_strong(
                    expected, desired, std::memory_order_acq_rel);
            }

           private:
            /**
             * @brief The atomic connection state.
             */
            std::atomic<e_state> m_state{e_state::idle};
        };

        /**
         * @struct cfg_t
         *
         * @brief Configuration parameters for Redis connection.
         *
         * @details Contains all necessary configuration options for establishing
         * and maintaining a Redis connection, including authentication, timeouts,
         * and connection parameters.
         *
         * @internal
         */
        struct cfg_t {
            /**
             * @brief The Redis server hostname or IP address.
             */
            std::string m_host{"127.0.0.1"};

            /**
             * @brief The Redis server port.
             */
            std::string m_port{"6379"};

            /**
             * @brief The username for Redis authentication.
             */
            std::string m_username{"default"};

            /**
             * @brief The password for Redis authentication.
             */
            std::string m_password{};

            /**
             * @brief The client name identifier.
             */
            std::string m_client_name{"client-name"};

            /**
             * @brief The unique client UUID.
             */
            std::string m_uuid{"client-uuid"};

            /**
             * @brief The Redis database number to select.
             */
            std::int32_t m_db{0};

            /**
             * @brief The connection timeout duration.
             */
            std::chrono::seconds m_connect_timeout{5};

            /**
             * @brief The interval for health check operations.
             */
            std::chrono::seconds m_health_check_interval{30};

            /**
             * @brief The wait interval before attempting reconnection.
             */
            std::chrono::seconds m_reconnect_wait_interval{1};

            /**
             * @brief The logging level for Redis operations.
             */
            boost::redis::logger::level m_log_level{boost::redis::logger::level::info};

            /**
             * @brief Whether to use SSL/TLS for the connection.
             */
            bool m_use_ssl{false};
        };

       public:
        CLUEAPI_INLINE base_connection_t() = delete;

        /**
         * @brief Constructs a Redis connection with the given configuration.
         *
         * @param cfg The connection configuration.
         * @param io_ctx The Boost.Asio I/O context for async operations.
         */
        CLUEAPI_INLINE base_connection_t(cfg_t cfg, boost::asio::io_context& io_ctx)
            : m_cfg{std::move(cfg)},
              m_state{state_t::idle},
              m_io_ctx{&io_ctx},
              m_connection{std::make_shared<boost::redis::connection>(io_ctx)} {
        }

        /**
         * @brief Destructs the connection, ensuring proper cleanup.
         */
        CLUEAPI_INLINE ~base_connection_t() {
            disconnect();
        }

       public:
        /**
         * @brief Disconnects from the Redis server and cleans up resources.
         */
        void disconnect();

       public:
        /**
         * @brief Checks if the connection is alive and ready for operations.
         *
         * @return `true` if the connection is alive, `false` otherwise.
         */
        CLUEAPI_INLINE bool is_alive() const noexcept {
            return m_state.get() == state_t::connected;
        }

        /**
         * @brief Gets the connection configuration.
         *
         * @return The connection configuration.
         */
        CLUEAPI_INLINE const auto& cfg() const noexcept {
            return m_cfg;
        }

        /**
         * @brief Gets the current connection state.
         *
         * @return The current connection state.
         */
        CLUEAPI_INLINE const auto state() const noexcept {
            return m_state.get();
        }

        /**
         * @brief Gets the underlying raw Redis connection.
         *
         * @return A shared pointer to the raw connection.
         */
        CLUEAPI_INLINE auto raw_connection() const noexcept {
            return m_connection;
        }

       protected:
        /**
         * @brief Builds the raw configuration for the underlying Redis connection.
         */
        void build_raw_cfg();

        /**
         * @brief Cancels the underlying Redis connection.
         */
        void cancel_connection();

       protected:
        /**
         * @brief Reference to the I/O context for async operations.
         */
        boost::asio::io_context* m_io_ctx;

        /**
         * @brief The connection configuration.
         */
        cfg_t m_cfg;

        /**
         * @brief The raw Redis configuration.
         */
        boost::redis::config m_raw_cfg;

        /**
         * @brief The current connection state.
         */
        state_t m_state;

        /**
         * @brief Whether the connection has been cancelled.
         */
        std::atomic_bool m_is_cancelled{false};

        /**
         * @brief The underlying Redis connection.
         */
        std::shared_ptr<raw_connection_t> m_connection;
    };
} // namespace clueapi::modules::redis::detail

#endif // CLUEAPI_MODULES_REDIS_DETAIL_BASE_CONNECTION_HXX