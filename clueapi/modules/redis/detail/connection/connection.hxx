/**
 * @file connection.hxx
 *
 * @brief Defines the asynchronous/synchronous Redis connection wrapper.
 *
 * @details This file provides a high-level, coroutine-based and blocking interfaces for interacting
 * with a Redis server. It inherits from `base_connection_t` and adds asynchronous
 * command execution.
 *
 * @internal
 */

#ifndef CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX
#define CLUEAPI_MODULES_REDIS_DETAIL_CONNECTION_HXX

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/redis/request.hpp>
#include <boost/redis/response.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include "clueapi/modules/redis/detail/base_connection/base_connection.hxx"

#include "clueapi/shared/macros.hxx"
#include "clueapi/shared/shared.hxx"

namespace clueapi::modules::redis::detail {
    /**
     * @struct connection_t
     *
     * @brief A Redis connection wrapper for asynchronous/synchronous operations.
     *
     * @details Provides a coroutine-based and blocking API for executing Redis commands.
     *
     * @internal
     */
    struct connection_t : base_connection_t {
        using base_t = base_connection_t;

        CLUEAPI_INLINE connection_t() = delete;

        /**
         * @brief Constructs a Redis connection with the given configuration.
         *
         * @param cfg The connection configuration.
         * @param io_ctx The Boost.Asio I/O context for operations.
         */
        CLUEAPI_INLINE connection_t(cfg_t cfg, boost::asio::io_context& io_ctx)
            : base_t{std::move(cfg), io_ctx} {
        }

       public:
        /**
         * @brief Establishes asynchronously a connection to the Redis server.
         *
         * @return An awaitable that resolves to `true` if connection was successful, `false`
         * otherwise.
         */
        shared::awaitable_t<bool> async_connect();

        /**
         * @brief Checks asynchronously if the connection is alive and responsive.
         *
         * @return An awaitable that resolves to `true` if the connection is alive, `false`
         * otherwise.
         */
        shared::awaitable_t<bool> async_check_alive();

        /**
         * @brief Establishes synchronously a connection to the Redis server.
         *
         * @return A `true` if connection was successful, `false` otherwise.
         */
        bool sync_connect();

        /**
         * @brief Checks synchronously if the connection is alive and responsive.
         *
         * @return A`true` if the connection is alive, `false` otherwise.
         */
        bool sync_check_alive();

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
         * @brief Executes a Redis request synchronously.
         *
         * @tparam _tuple_t The response tuple types.
         *
         * @param req The Redis request to execute.
         * @param resp The response object to populate.
         *
         * @return A error code indicating the result of the operation.
         */
        template <typename... _tuple_t>
        CLUEAPI_NOINLINE boost::system::error_code exec(
            const boost::redis::request& req, boost::redis::response<_tuple_t...>& resp) {
            boost::system::error_code ec{};

            try {
                auto fut = m_connection->async_exec(req, resp, boost::asio::use_future);

                fut.get();
            } catch (const boost::system::system_error& e) {
                ec = e.code();
            }

            return ec;
        }

       public:
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
        CLUEAPI_NOINLINE shared::awaitable_t<std::optional<_type_t>> async_get(
            const std::string_view& key) {
            boost::redis::request req{};

            req.push("GET", key);

            boost::redis::response<std::optional<_type_t>> resp{};

            auto ec = co_await async_exec(req, resp);

            if (ec)
                co_return std::nullopt;

            co_return std::get<0>(resp).value();
        }

        /**
         * @brief Gets a value from Redis by key.
         *
         * @tparam _type_t The expected type of the value.
         *
         * @param key The key to retrieve.
         *
         * @return A value if found, `std::nullopt` otherwise.
         */
        template <typename _type_t>
        CLUEAPI_NOINLINE std::optional<_type_t> sync_get(const std::string_view& key) {
            boost::redis::request req{};

            req.push("GET", key);

            boost::redis::response<std::optional<_type_t>> resp{};

            auto ec = exec(req, resp);

            if (ec)
                return std::nullopt;

            return std::get<0>(resp).value();
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
        shared::awaitable_t<bool> async_set(
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
        shared::awaitable_t<bool> async_del(const std::string_view& key);

        /**
         * @brief Checks if a key exists in Redis.
         *
         * @param key The key to check.
         *
         * @return An awaitable that resolves to `true` if the key exists, `false` otherwise.
         */
        shared::awaitable_t<bool> async_exists(const std::string_view& key);

        /**
         * @brief Sets the TTL for a key.
         *
         * @param key The key to set expiration for.
         * @param ttl The time-to-live duration.
         *
         * @return An awaitable that resolves to `true` if successful, `false` otherwise.
         */
        shared::awaitable_t<bool> async_expire(
            const std::string_view& key, std::chrono::seconds ttl);

        /**
         * @brief Gets the TTL of a key.
         *
         * @param key The key to check.
         *
         * @return An awaitable that resolves to the TTL in seconds, or -1 if no TTL is set.
         */
        shared::awaitable_t<std::int32_t> async_ttl(const std::string_view& key);

        /**
         * @brief Pushes a value to the left of a list.
         *
         * @param key The list key.
         * @param value The value to push.
         *
         * @return An awaitable that resolves to the new length of the list.
         */
        shared::awaitable_t<std::int32_t> async_lpush(
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
        shared::awaitable_t<bool> async_ltrim(
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
        shared::awaitable_t<std::vector<std::string>> async_lrange(
            const std::string_view& key, std::int32_t start, std::int32_t end);

        /**
         * @brief Sets multiple fields in a hash.
         *
         * @param key The hash key.
         * @param mapping The field-value mappings to set.
         *
         * @return An awaitable that resolves to the number of fields that were added.
         */
        shared::awaitable_t<std::int32_t> async_hset(
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
        shared::awaitable_t<std::int32_t> async_hdel(
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
        shared::awaitable_t<std::int32_t> async_hsetfield(
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
        shared::awaitable_t<std::unordered_map<std::string, std::string>> async_hgetall(
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
        shared::awaitable_t<std::int32_t> async_hincrby(
            const std::string_view& key, const std::string_view& field, std::int32_t increment);

        /**
         * @brief Gets the value of a hash field.
         *
         * @param key The hash key.
         * @param field The field name.
         *
         * @return An awaitable that resolves to the field value if found, `std::nullopt` otherwise.
         */
        shared::awaitable_t<std::optional<std::string>> async_hget(
            const std::string_view& key, const std::string_view& field);

        /**
         * @brief Checks if a hash field exists.
         *
         * @param key The hash key.
         * @param field The field name.
         *
         * @return An awaitable that resolves to `true` if the field exists, `false` otherwise.
         */
        shared::awaitable_t<bool> async_hexists(
            const std::string_view& key, const std::string_view& field);

        /**
         * @brief Increments a key's value by 1.
         *
         * @param key The key to increment.
         *
         * @return An awaitable that resolves to the new value if successful, `std::nullopt`
         * otherwise.
         */
        shared::awaitable_t<std::optional<std::int32_t>> async_incr(const std::string_view& key);

        /**
         * @brief Decrements a key's value by 1.
         *
         * @param key The key to decrement.
         *
         * @return An awaitable that resolves to the new value if successful, `std::nullopt`
         * otherwise.
         */
        shared::awaitable_t<std::optional<std::int32_t>> async_decr(const std::string_view& key);

       public:
        /**
         * @brief Sets a key-value pair in Redis with optional TTL.
         *
         * @param key The key to set.
         * @param value The value to associate with the key.
         * @param ttl The time-to-live for the key (0 for no expiration).
         *
         * @return A `true` if successful, `false` otherwise.
         */
        bool sync_set(
            const std::string_view& key,
            const std::string_view& value,

            std::chrono::seconds ttl = std::chrono::seconds{0});

        /**
         * @brief Deletes a key from Redis.
         *
         * @param key The key to delete.
         *
         * @return A `true` if the key was deleted, `false` otherwise.
         */
        bool sync_del(const std::string_view& key);

        /**
         * @brief Checks if a key exists in Redis.
         *
         * @param key The key to check.
         *
         * @return A `true` if the key exists, `false` otherwise.
         */
        bool sync_exists(const std::string_view& key);

        /**
         * @brief Sets the TTL for a key.
         *
         * @param key The key to set expiration for.
         * @param ttl The time-to-live duration.
         *
         * @return A `true` if successful, `false` otherwise.
         */
        bool sync_expire(const std::string_view& key, std::chrono::seconds ttl);

        /**
         * @brief Gets the TTL of a key.
         *
         * @param key The key to check.
         *
         * @return A TTL in seconds, or -1 if no TTL is set.
         */
        std::int32_t sync_ttl(const std::string_view& key);

        /**
         * @brief Pushes a value to the left of a list.
         *
         * @param key The list key.
         * @param value The value to push.
         *
         * @return A new length of the list.
         */
        std::int32_t sync_lpush(const std::string_view& key, const std::string_view& value);

        /**
         * @brief Trims a list to the specified range.
         *
         * @param key The list key.
         * @param start The start index (inclusive).
         * @param end The end index (inclusive).
         *
         * @return A `true` if successful, `false` otherwise.
         */
        bool sync_ltrim(const std::string_view& key, std::int32_t start, std::int32_t end);

        /**
         * @brief Gets a range of elements from a list.
         *
         * @param key The list key.
         * @param start The start index (inclusive).
         * @param end The end index (inclusive).
         *
         * @return A vector of list elements.
         */
        std::vector<std::string> sync_lrange(
            const std::string_view& key, std::int32_t start, std::int32_t end);

        /**
         * @brief Sets multiple fields in a hash.
         *
         * @param key The hash key.
         * @param mapping The field-value mappings to set.
         *
         * @return A number of fields that were added.
         */
        std::int32_t sync_hset(
            const std::string_view& key,
            const std::unordered_map<std::string_view, std::string_view>& mapping);

        /**
         * @brief Deletes fields from a hash.
         *
         * @param key The hash key.
         * @param fields The fields to delete.
         *
         * @return A number of fields that were removed.
         */
        std::int32_t sync_hdel(
            const std::string_view& key, const std::vector<std::string_view>& fields);

        /**
         * @brief Sets a single field in a hash.
         *
         * @param key The hash key.
         * @param field The field name.
         * @param value The field value.
         *
         * @return A 1 if a new field was created, 0 if updated.
         */
        std::int32_t sync_hsetfield(
            const std::string_view& key,
            const std::string_view& field,
            const std::string_view& value);

        /**
         * @brief Gets all fields and values from a hash.
         *
         * @param key The hash key.
         *
         * @return A map of all field-value pairs.
         */
        std::unordered_map<std::string, std::string> sync_hgetall(const std::string_view& key);

        /**
         * @brief Increments a hash field by the given amount.
         *
         * @param key The hash key.
         * @param field The field name.
         * @param increment The amount to increment by.
         *
         * @return A new value of the field.
         */
        std::int32_t sync_hincrby(
            const std::string_view& key, const std::string_view& field, std::int32_t increment);

        /**
         * @brief Gets the value of a hash field.
         *
         * @param key The hash key.
         * @param field The field name.
         *
         * @return A field value if found, `std::nullopt` otherwise.
         */
        std::optional<std::string> sync_hget(
            const std::string_view& key, const std::string_view& field);

        /**
         * @brief Checks if a hash field exists.
         *
         * @param key The hash key.
         * @param field The field name.
         *
         * @return A `true` if the field exists, `false` otherwise.
         */
        bool sync_hexists(const std::string_view& key, const std::string_view& field);

        /**
         * @brief Increments a key's value by 1.
         *
         * @param key The key to increment.
         *
         * @return A new value if successful, `std::nullopt`
         * otherwise.
         */
        std::optional<std::int32_t> sync_incr(const std::string_view& key);

        /**
         * @brief Decrements a key's value by 1.
         *
         * @param key The key to decrement.
         *
         * @return A new value if successful, `std::nullopt`
         * otherwise.
         */
        std::optional<std::int32_t> sync_decr(const std::string_view& key);
    };
} // namespace clueapi::modules::redis::detail

#endif // CLUEAPI_MODULES_REDIS_DETAIL_ASYNC_CONNECTION_HXX