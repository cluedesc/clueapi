/**
 * @file logging.hxx
 *
 * @brief The main public header for the clueapi logging module.
 */

#ifndef CLUEAPI_MODULES_LOGGING_HXX
#define CLUEAPI_MODULES_LOGGING_HXX

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <utility>

#include "clueapi/shared/macros.hxx"
#include "clueapi/shared/shared.hxx"

#include "detail/base_logger/base_logger.hxx"
#include "detail/hash/hash.hxx"
#include "detail/level/level.hxx"
#include "detail/log/log.hxx"

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging {
    /**
     * @brief A type alias for the log level enumeration.
     */
    using e_log_level = detail::e_log_level;

    /**
     * @brief A type alias for the log message structure.
     */
    using log_msg_t = detail::log_msg_t;

    /**
     * @brief A type alias for the logger parameters structure.
     */
    using logger_params_t = detail::logger_params_t;

    /**
     * @brief A type alias for the base logger class.
     */
    using base_logger_t = detail::base_logger_t;

    /**
     * @struct cfg_t
     *
     * @brief Configuration for the logging system.
     */
    struct cfg_t {
        /**
         * @brief Enables or disables asynchronous logging.
         */
        bool m_async_mode{false};

        /**
         * @brief The sleep duration for the async logger thread.
         */
        std::chrono::milliseconds m_sleep{100};

        /**
         * @brief The default log level for all loggers.
         */
        e_log_level m_default_level{e_log_level::info};
    };

    /**
     * @class c_logging
     *
     * @brief Manages all loggers and the logging process.
     */
    class c_logging {
       public:
        CLUEAPI_INLINE c_logging() noexcept = default;

        CLUEAPI_INLINE ~c_logging() {
            destroy();
        }

       public:
        /**
         * @brief Initializes the logging system.
         *
         * @param cfg The configuration for the logging system.
         */
        void init(cfg_t cfg);

        /**
         * @brief Destroys the logging system, cleaning up resources.
         */
        void destroy();

       public:
        /**
         * @brief Adds a logger to the system.
         *
         * @param hash The hash of the logger's name.
         * @param logger The logger instance to add.
         */
        void add_logger(detail::hash_t hash, std::shared_ptr<base_logger_t> logger);

        /**
         * @brief Removes a logger from the system.
         *
         * @param hash The hash of the logger's name to remove.
         */
        void remove_logger(detail::hash_t hash);

        /**
         * @brief Retrieves a logger from the system.
         *
         * @param hash The hash of the logger's name.
         *
         * @return A shared pointer to the logger, or nullptr if not found.
         */
        std::shared_ptr<base_logger_t> get_logger(detail::hash_t hash);

       public:
        /**
         * @brief Checks if the logging system is running.
         *
         * @param m The memory order for the atomic load.
         *
         * @return `true` if running, `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE auto is_running(
            std::memory_order m = std::memory_order_relaxed) const noexcept {
            return m_is_running.load(m);
        }

        /**
         * @brief Sets the default log level.
         *
         * @param level The new default log level.
         */
        CLUEAPI_INLINE void set_default_level(e_log_level level) noexcept {
            m_cfg.m_default_level = level;
        }

        /**
         * @brief Gets the default log level.
         *
         * @return The default log level.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& default_level() const noexcept {
            return m_cfg.m_default_level;
        }

       private:
        /**
         * @brief The main loop for asynchronous processing of log messages.
         */
        void process_async();

       private:
        /**
         * @brief A flag indicating if the async logging system is running.
         */
        std::atomic_bool m_is_running{false};

        /**
         * @brief The configuration for the logging system.
         */
        cfg_t m_cfg{};

        /**
         * @brief The thread for the async logging system.
         */
        std::thread m_thread;

        /**
         * @brief A mutex for thread-safe access to the loggers map.
         */
        std::shared_mutex m_mutex;

        /**
         * @brief A condition variable for the async logging system.
         */
        std::shared_ptr<std::condition_variable_any> m_condition;

        /**
         * @brief The internal map of loggers.
         */
        shared::unordered_map_t<detail::hash_t, std::shared_ptr<base_logger_t>> m_loggers;
    };

    /**
     * @brief A dispatch function for logging messages.
     *
     * @tparam _args_t The types of the format arguments.
     * @tparam _logging_t The type of the logging system instance.
     *
     * @param logging The logging system instance.
     * @param hash The hash of the logger's name.
     * @param level The log level.
     * @param str The format string.
     * @param args The format arguments.
     */
    template <typename... _args_t, typename _logging_t>
    CLUEAPI_INLINE void log_dispatch(
        _logging_t&& logging,

        detail::hash_t hash,

        e_log_level level,

        std::string_view str,

        _args_t&&... args) {
        log_msg_t msg{
            .m_msg = fmt::format(fmt::runtime(str), std::forward<_args_t>(args)...),

            .m_level = level,

            .m_time = std::chrono::system_clock::now()};

        if constexpr (std::is_pointer_v<std::remove_reference_t<_logging_t>>) {
            auto logger = logging->get_logger(hash);

            if (!logger)
                return;

            return logger->log(std::move(msg));
        } else {
            auto logger = logging.get_logger(hash);

            if (!logger)
                return;

            logger->log(std::move(msg));
        }
    }
} // namespace clueapi::modules::logging

#include "loggers/loggers.hxx"

/**
 * @def CLUEAPI_LOG_IMPL
 *
 * @brief The implementation macro for logging.
 */
#define CLUEAPI_LOG_IMPL(main_logging, hash, level, message, ...) \
    (::clueapi::modules::logging::log_dispatch(                   \
        main_logging, hash, level, message, ##__VA_ARGS__))

#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_HXX