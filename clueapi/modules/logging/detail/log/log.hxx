/**
 * @file log.hxx
 *
 * @brief Defines the structure for a log message.
 */

#ifndef CLUEAPI_MODULES_LOGGING_DETAIL_LOG_HXX
#define CLUEAPI_MODULES_LOGGING_DETAIL_LOG_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging::detail {
    /**
     * @struct log_msg_t
     *
     * @brief Represents a single log message.
     *
     * @internal
     */
    struct log_msg_t {
        /**
         * @brief The log message.
         */
        std::string m_msg;

        /**
         * @brief The log level.
         */
        e_log_level m_level{};

        /**
         * @brief The timestamp of the log message.
         */
        std::chrono::system_clock::time_point m_time;
    };
} // namespace clueapi::modules::logging::detail
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_DETAIL_LOG_HXX