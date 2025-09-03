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
         * @brief The logging level for Redis operations.
         */
        boost::redis::logger::level m_log_level{boost::redis::logger::level::info};
    };
} // namespace clueapi::modules::redis::detail

#endif // CLUEAPI_MODULES_REDIS_DETAIL_CFG_HXX