/**
 * @file connection.hxx
 *
 * @brief Defines the Redis connection management for the Redis module.
 *
 * @details This file provides a wrapper around boost::redis::connection with
 * automatic connection management, health checking, and high-level Redis operations.
 */

#ifndef CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX
#define CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX

namespace clueapi::modules::redis::detail {
    /**
     * @brief A type alias for the raw Redis connection.
     *
     * @internal
     */
    using raw_connection_t = std::shared_ptr<boost::redis::connection>;

    /**
     * @struct connection_t
     *
     * @brief Manages Redis connections with automatic reconnection and state tracking.
     *
     * @details This class provides a high-level interface for Redis operations with
     * built-in connection management, health checking, and automatic reconnection.
     * It supports both basic Redis operations and advanced features like hash operations
     * and list operations.
     *
     * @internal
     */
    struct connection_t {
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
        CLUEAPI_INLINE connection_t() = delete;

        /**
         * @brief Constructs a Redis connection with the given configuration.
         *
         * @param cfg The connection configuration.
         * @param io_ctx The Boost.Asio I/O context for async operations.
         */
        CLUEAPI_INLINE connection_t(cfg_t cfg, boost::asio::io_context& io_ctx)
            : m_cfg{std::move(cfg)},
              m_state{state_t::idle},
              m_io_ctx{io_ctx},
              m_connection{std::make_shared<boost::redis::connection>(io_ctx)} {
        }

        /**
         * @brief Destructs the connection, ensuring proper cleanup.
         */
        CLUEAPI_INLINE ~connection_t() {
            disconnect();
        }

       public:
        /**
         * @brief Establishes a connection to the Redis server.
         *
         * @return An awaitable that resolves to `true` if connection was successful, `false`
         * otherwise.
         */
        shared::awaitable_t<bool> connect();

        /**
         * @brief Checks if the connection is alive and responsive.
         *
         * @return An awaitable that resolves to `true` if the connection is alive, `false`
         * otherwise.
         */
        shared::awaitable_t<bool> check_alive();

       public:
        /**
         * @brief Disconnects from the Redis server and cleans up resources.
         */
        void disconnect();

       public:
        /**
         * @brief Executes a Redis request asynchronously.
         *
         * @tparam _tuple_t The response tuple types.
         *
         * @param req The Redis request to execute.
         * @param resp The response object to populate.
         *
         * @return An awaitable that resolves to the error code of the operation.
         */
        template <typename... _tuple_t>
        CLUEAPI_NOINLINE shared::awaitable_t<boost::system::error_code> async_exec(
            const boost::redis::request& req, boost::redis::response<_tuple_t...>& resp) {
            boost::system::error_code ec{};

            co_await m_connection->async_exec(
                req, resp, boost::asio::redirect_error(boost::asio::use_awaitable, ec));

            co_return ec;
        }

        /**
         * @brief Gets a value from Redis by key.
         *
         * @tparam _type_t The expected type of the value.
         *
         * @param key The key to retrieve.
         *
         * @return An awaitable that resolves to the value if found, `std::nullopt` otherwise.
         */
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
        /**
         * @brief Sets a key-value pair in Redis with optional TTL.
         *
         * @param key The key to set.
         * @param value The value to associate with the key.
         * @param ttl The time-to-live for the key (0 for no expiration).
         *
         * @return An awaitable that resolves to `true` if successful, `false` otherwise.
         */
        shared::awaitable_t<bool> set(
            const std::string_view& key,
            const std::string_view& value,

            std::chrono::seconds ttl = std::chrono::seconds{0});

        /**
         * @brief Deletes a key from Redis.
         *
         * @param key The key to delete.
         *
         * @return An awaitable that resolves to `true` if the key was deleted, `false` otherwise.
         */
        shared::awaitable_t<bool> del(const std::string_view& key);

        /**
         * @brief Checks if a key exists in Redis.
         *
         * @param key The key to check.
         *
         * @return An awaitable that resolves to `true` if the key exists, `false` otherwise.
         */
        shared::awaitable_t<bool> exists(const std::string_view& key);

        /**
         * @brief Sets the TTL for a key.
         *
         * @param key The key to set expiration for.
         * @param ttl The time-to-live duration.
         *
         * @return An awaitable that resolves to `true` if successful, `false` otherwise.
         */
        shared::awaitable_t<bool> expire(const std::string_view& key, std::chrono::seconds ttl);

        /**
         * @brief Gets the TTL of a key.
         *
         * @param key The key to check.
         *
         * @return An awaitable that resolves to the TTL in seconds, or -1 if no TTL is set.
         */
        shared::awaitable_t<std::int32_t> ttl(const std::string_view& key);

        /**
         * @brief Pushes a value to the left of a list.
         *
         * @param key The list key.
         * @param value The value to push.
         *
         * @return An awaitable that resolves to the new length of the list.
         */
        shared::awaitable_t<std::int32_t> lpush(
            const std::string_view& key, const std::string_view& value);

        /**
         * @brief Trims a list to the specified range.
         *
         * @param key The list key.
         * @param start The start index (inclusive).
         * @param end The end index (inclusive).
         *
         * @return An awaitable that resolves to `true` if successful, `false` otherwise.
         */
        shared::awaitable_t<bool> ltrim(
            const std::string_view& key, std::int32_t start, std::int32_t end);

        /**
         * @brief Gets a range of elements from a list.
         *
         * @param key The list key.
         * @param start The start index (inclusive).
         * @param end The end index (inclusive).
         *
         * @return An awaitable that resolves to a vector of list elements.
         */
        shared::awaitable_t<std::vector<std::string>> lrange(
            const std::string_view& key, std::int32_t start, std::int32_t end);

        /**
         * @brief Sets multiple fields in a hash.
         *
         * @param key The hash key.
         * @param mapping The field-value mappings to set.
         *
         * @return An awaitable that resolves to the number of fields that were added.
         */
        shared::awaitable_t<std::int32_t> hset(
            const std::string_view& key,
            const std::unordered_map<std::string_view, std::string_view>& mapping);

        /**
         * @brief Deletes fields from a hash.
         *
         * @param key The hash key.
         * @param fields The fields to delete.
         *
         * @return An awaitable that resolves to the number of fields that were removed.
         */
        shared::awaitable_t<std::int32_t> hdel(
            const std::string_view& key, const std::vector<std::string_view>& fields);

        /**
         * @brief Sets a single field in a hash.
         *
         * @param key The hash key.
         * @param field The field name.
         * @param value The field value.
         *
         * @return An awaitable that resolves to 1 if a new field was created, 0 if updated.
         */
        shared::awaitable_t<std::int32_t> hsetfield(
            const std::string_view& key,
            const std::string_view& field,
            const std::string_view& value);

        /**
         * @brief Gets all fields and values from a hash.
         *
         * @param key The hash key.
         *
         * @return An awaitable that resolves to a map of all field-value pairs.
         */
        shared::awaitable_t<std::unordered_map<std::string, std::string>> hgetall(
            const std::string_view& key);

        /**
         * @brief Increments a hash field by the given amount.
         *
         * @param key The hash key.
         * @param field The field name.
         * @param increment The amount to increment by.
         *
         * @return An awaitable that resolves to the new value of the field.
         */
        shared::awaitable_t<std::int32_t> hincrby(
            const std::string_view& key, const std::string_view& field, std::int32_t increment);

        /**
         * @brief Gets the value of a hash field.
         *
         * @param key The hash key.
         * @param field The field name.
         *
         * @return An awaitable that resolves to the field value if found, `std::nullopt` otherwise.
         */
        shared::awaitable_t<std::optional<std::string>> hget(
            const std::string_view& key, const std::string_view& field);

        /**
         * @brief Checks if a hash field exists.
         *
         * @param key The hash key.
         * @param field The field name.
         *
         * @return An awaitable that resolves to `true` if the field exists, `false` otherwise.
         */
        shared::awaitable_t<bool> hexists(
            const std::string_view& key, const std::string_view& field);

        /**
         * @brief Increments a key's value by 1.
         *
         * @param key The key to increment.
         *
         * @return An awaitable that resolves to the new value if successful, `std::nullopt`
         * otherwise.
         */
        shared::awaitable_t<std::optional<std::int32_t>> incr(const std::string_view& key);

        /**
         * @brief Decrements a key's value by 1.
         *
         * @param key The key to decrement.
         *
         * @return An awaitable that resolves to the new value if successful, `std::nullopt`
         * otherwise.
         */
        shared::awaitable_t<std::optional<std::int32_t>> decr(const std::string_view& key);

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
        CLUEAPI_INLINE state_t::e_state state() const noexcept {
            return m_state.get();
        }

        /**
         * @brief Gets the underlying raw Redis connection.
         *
         * @return A shared pointer to the raw connection.
         */
        CLUEAPI_INLINE raw_connection_t raw_connection() const noexcept {
            return m_connection;
        }

       private:
        /**
         * @brief Builds the raw configuration for the underlying Redis connection.
         */
        void build_raw_cfg();

        /**
         * @brief Cancels the underlying Redis connection.
         */
        void cancel_connection();

       private:
        /**
         * @brief Reference to the I/O context for async operations.
         */
        boost::asio::io_context& m_io_ctx;

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
        raw_connection_t m_connection;
    };
} // namespace clueapi::modules::redis::detail

#endif // CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX