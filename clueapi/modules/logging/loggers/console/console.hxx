/**
 * @file console.hxx
 *
 * @brief Defines the console logger.
 */

#ifndef CLUEAPI_MODULES_LOGGING_LOGGERS_CONSOLE_HXX
#define CLUEAPI_MODULES_LOGGING_LOGGERS_CONSOLE_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include <utility>

#include "clueapi/modules/logging/detail/base_logger/base_logger.hxx"

#include "clueapi/shared/macros.hxx"

namespace clueapi::modules::logging {
    /**
     * @struct console_logger_t
     *
     * @brief A logger that writes messages to the console.
     */
    struct console_logger_t final : detail::base_logger_t {
        /**
         * @brief Constructs a console logger with the given parameters.
         *
         * @param params The parameters for the logger.
         */
        CLUEAPI_INLINE console_logger_t(detail::logger_params_t params) noexcept
            : base_logger_t{std::move(params)} {
        }

        CLUEAPI_INLINE ~console_logger_t() noexcept {
            m_buffer.clear();
        }

       public:
        /**
         * @brief Logs a message to the console.
         *
         * @param msg The log message.
         */
        void log(detail::log_msg_t msg) override;

        /**
         * @brief Processes a batch of log messages.
         */
        void process() override;

        /**
         * @brief Handles buffer overflow.
         *
         * @param msg The message that caused the overflow.
         */
        void handle_overflow(detail::log_msg_t msg) override;
    };
} // namespace clueapi::modules::logging
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_LOGGERS_CONSOLE_HXX