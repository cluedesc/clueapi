/**
 * @file logging.hxx
 *
 * @brief Defines the configuration structure for the clueapi logging module.
 */

#ifndef CLUEAPI_CFG_LOGGING_HXX
#define CLUEAPI_CFG_LOGGING_HXX

#include <chrono>
#include <cstddef>
#include <string>

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include "clueapi/modules/logging/logging.hxx"
#endif // CLUEAPI_USE_LOGGING_MODULE

namespace clueapi::cfg {
#ifdef CLUEAPI_USE_LOGGING_MODULE
    /**
     * @struct logging_cfg_t
     *
     * @brief Configuration settings for the logging system.
     *
     * @note These settings are only used if the logging module is enabled via
     * `CLUEAPI_USE_LOGGING_MODULE`.
     */
    struct logging_cfg_t {
        /**
         * @brief Enables or disables asynchronous logging mode.
         *
         * @note When `true`, log messages are placed in a queue and processed by a dedicated
         * background thread. This improves application performance by offloading I/O operations.
         * When `false`, log messages are processed synchronously, which may block the calling
         * thread.
         */
        bool m_async_mode{false};

        /**
         * @brief The polling interval for the asynchronous logger's worker thread.
         *
         * @note In async mode, this is the duration the worker thread sleeps before checking for
         * new messages. Only applicable if `m_async_mode` is `true`.
         */
        std::chrono::milliseconds m_sleep{100};

        /**
         * @brief The default logging level. Messages with a severity lower than this will be
         * ignored.
         */
        modules::logging::e_log_level m_default_level{modules::logging::e_log_level::info};

        /**
         * @brief The name of the logger instance.
         *
         * @note This name is often used as a prefix in log messages or as part of the log filename.
         */
        std::string m_name{"clueapi"};

        /**
         * @brief The maximum number of log messages that can be queued in asynchronous mode.
         *
         * @warning If the queue is full, new log messages are typically dropped to prevent the
         * application from blocking. This is a critical parameter for balancing performance and log
         * reliability under high load. Only applicable if `m_async_mode` is `true`.
         */
        std::size_t m_capacity{8192};

        /**
         * @brief The maximum number of messages to process in a single batch in asynchronous mode.
         *
         * @note The worker thread will process up to this many messages from the queue in one go
         * before sleeping. Only applicable if `m_async_mode` is `true`.
         */
        std::size_t m_batch_size{512};
    };
#endif // CLUEAPI_USE_LOGGING_MODULE
} // namespace clueapi::cfg

#endif // CLUEAPI_CFG_LOGGING_HXX