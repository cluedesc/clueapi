/**
 * @file base_logger.hxx
 *
 * @brief Defines the base class for all loggers.
 */

#ifndef CLUEAPI_MODULES_LOGGING_DETAIL_BASE_LOGGER_HXX
#define CLUEAPI_MODULES_LOGGING_DETAIL_BASE_LOGGER_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging {
    class c_logging;

    namespace detail {
        /**
         * @struct logger_params_t
         *
         * @brief Defines the parameters for a logger.
         *
         * @internal
         */
        struct logger_params_t {
            /**
             * @brief The name of the logger.
             */
            std::string m_name;

            /**
             * @brief The minimum log level to process.
             */
            e_log_level m_level{e_log_level::info};

            /**
             * @brief The capacity of the logger's message buffer.
             */
            std::size_t m_capacity{2048};

            /**
             * @brief The size of batches to process in async mode.
             */
            std::size_t m_batch_size{256};

            /**
             * @brief Enables or disables asynchronous logging.
             */
            bool m_async_mode{false};
        };

        /**
         * @struct prv_logger_params_t
         *
         * @brief Defines private parameters for a logger, set by the logging system.
         *
         * @internal
         */
        struct prv_logger_params_t {
            /**
             * @brief A condition variable for asynchronous logging.
             */
            std::shared_ptr<std::condition_variable_any> m_condition;
        };

        /**
         * @struct base_logger_t
         *
         * @brief An abstract base class for all loggers.
         *
         * @internal
         */
        struct base_logger_t {
            CLUEAPI_INLINE base_logger_t() noexcept = default;

            /**
             * @brief Constructs a base logger with the given parameters.
             *
             * @param params The parameters for the logger.
             */
            CLUEAPI_INLINE base_logger_t(logger_params_t params) noexcept
                : m_params{std::move(params)}, m_buffer{m_params.m_capacity}, m_enabled{true} {
            }

            CLUEAPI_INLINE virtual ~base_logger_t() noexcept = default;

           public:
            /**
             * @brief Logs a message.
             *
             * @param msg The log message to process.
             */
            virtual void log(log_msg_t msg) = 0;

            /**
             * @brief Processes a batch of log messages.
             */
            virtual void process() = 0;

            /**
             * @brief Handles buffer overflow.
             *
             * @param msg The message that caused the overflow.
             */
            virtual void handle_overflow(log_msg_t msg) = 0;

           public:
            /**
             * @brief Enables or disables the logger.
             *
             * @param enabled `true` to enable, `false` to disable.
             */
            CLUEAPI_INLINE void set_enabled(bool enabled) noexcept {
                m_enabled = enabled;
            }

            /**
             * @brief Sets the log level.
             *
             * @param level The new log level.
             */
            CLUEAPI_INLINE void set_level(e_log_level level) noexcept {
                m_params.m_level = level;
            }

            /**
             * @brief Sets the buffer capacity.
             *
             * @param capacity The new capacity.
             */
            CLUEAPI_INLINE void set_capacity(std::size_t capacity) noexcept {
                m_buffer.set_capacity(capacity);
            }

            /**
             * @brief Sets the batch size for async processing.
             *
             * @param batch_size The new batch size.
             */
            CLUEAPI_INLINE void set_batch_size(std::size_t batch_size) noexcept {
                m_params.m_batch_size = batch_size;
            }

            /**
             * @brief Sets the asynchronous mode.
             *
             * @param async_mode `true` for async, `false` for sync.
             */
            CLUEAPI_INLINE void set_async_mode(bool async_mode) noexcept {
                if (async_mode && !m_params.m_async_mode)
                    m_buffer = msg_buffer_t{m_params.m_capacity};

                else if (!async_mode && m_params.m_async_mode)
                    m_buffer.destroy();

                m_params.m_async_mode = async_mode;
            }

            /**
             * @brief Gets the logger's message buffer.
             * @return A reference to the message buffer.
             */
            CLUEAPI_INLINE auto& buffer() noexcept {
                return m_buffer;
            }

            /**
             * @brief Gets the logger's message buffer.
             * @return A const reference to the message buffer.
             */
            CLUEAPI_INLINE const auto& buffer() const noexcept {
                return m_buffer;
            }

            /**
             * @brief Gets the logger's parameters.
             * @return A reference to the logger parameters.
             */
            CLUEAPI_INLINE auto& params() noexcept {
                return m_params;
            }

            /**
             * @brief Gets the logger's parameters.
             * @return A const reference to the logger parameters.
             */
            CLUEAPI_INLINE const auto& params() const noexcept {
                return m_params;
            }

            /**
             * @brief Checks if the logger is in async mode.
             * @return `true` if in async mode, `false` otherwise.
             */
            CLUEAPI_INLINE const auto& async_mode() const noexcept {
                return m_params.m_async_mode;
            }

            /**
             * @brief Checks if the logger is enabled.
             * @return `true` if enabled, `false` otherwise.
             */
            CLUEAPI_INLINE const auto& enabled() const noexcept {
                return m_enabled;
            }

            /**
             * @brief Gets the name of the logger.
             * @return The logger's name.
             */
            CLUEAPI_INLINE const auto& name() const noexcept {
                return m_params.m_name;
            }

            /**
             * @brief Gets the log level of the logger.
             * @return The logger's log level.
             */
            CLUEAPI_INLINE const auto& level() const noexcept {
                return m_params.m_level;
            }

           private:
            /**
             * @brief Sets the private parameters for the logger.
             *
             * @param prv_params The private parameters.
             */
            CLUEAPI_INLINE void set_prv_params(prv_logger_params_t prv_params) noexcept {
                m_prv_params = std::move(prv_params);
            }

            friend class clueapi::modules::logging::c_logging;

           protected:
            /**
             * @brief The parameters for the logger.
             */
            logger_params_t m_params{};

            /**
             * @brief Private parameters set by the logging system.
             */
            prv_logger_params_t m_prv_params{};

            /**
             * @brief The message buffer for the logger.
             */
            msg_buffer_t m_buffer;

            /**
             * @brief A flag indicating if the logger is enabled.
             */
            bool m_enabled{};
        };
    } // namespace detail
} // namespace clueapi::modules::logging
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_DETAIL_BASE_LOGGER_HXX