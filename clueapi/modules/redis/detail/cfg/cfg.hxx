/**
 * @file cfg.hxx
 *
 * @brief Defines the configuration structure for the Redis module.
 *
 * @details This file provides the basic configuration parameters needed
 * to establish a connection to a Redis server.
 */

#ifndef CLUEAPI_MODULES_REDIS_DETAIL_CFG_HXX
#define CLUEAPI_MODULES_REDIS_DETAIL_CFG_HXX

#include <chrono>
#include <cstdint>
#include <string>

#include <boost/redis/logger.hpp>

namespace clueapi::modules::redis::detail {
    /**
     * @struct cfg_t
     *
     * @brief Basic configuration parameters for Redis connection.
     *
     * @details Contains essential connection parameters for establishing
     * a Redis connection, including server details and authentication.
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
         * @brief The password for Redis authentication.
         */
        std::string m_password{};

        /**
         * @brief The username for Redis authentication.
         */
        std::string m_username{};

        /**
         * @brief The Redis database number to select.
         */
        std::int32_t m_db{0};

        /**
         * @brief Whether to use SSL/TLS for the connection.
         */
        bool m_use_ssl{false};

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
    };
} // namespace clueapi::modules::redis::detail

#endif // CLUEAPI_MODULES_REDIS_DETAIL_CFG_HXX