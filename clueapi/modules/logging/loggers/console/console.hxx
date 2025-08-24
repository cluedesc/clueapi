/**
 * @file console.hxx
 *
 * @brief Defines the console logger.
 */

#ifndef CLUEAPI_MODULES_LOGGING_LOGGERS_CONSOLE_HXX
#define CLUEAPI_MODULES_LOGGING_LOGGERS_CONSOLE_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging {
    /**
     * @struct console_logger_t
     *
     * @brief A logger that writes messages to the console.
     */
    struct console_logger_t final : base_logger_t {
        /**
         * @brief Constructs a console logger with the given parameters.
         *
         * @param params The parameters for the logger.
         */
        CLUEAPI_INLINE console_logger_t(logger_params_t params) noexcept
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
        void log(log_msg_t msg) override;

        /**
         * @brief Processes a batch of log messages.
         */
        void process() override;

        /**
         * @brief Handles buffer overflow.
         *
         * @param msg The message that caused the overflow.
         */
        void handle_overflow(log_msg_t msg) override;
    };
} // namespace clueapi::modules::logging
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_LOGGERS_CONSOLE_HXX