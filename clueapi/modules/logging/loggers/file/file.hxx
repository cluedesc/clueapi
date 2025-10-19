/**
 * @file file.hxx
 *
 * @brief Defines the file logger.
 */

#ifndef CLUEAPI_MODULES_LOGGING_LOGGERS_FILE_HXX
#define CLUEAPI_MODULES_LOGGING_LOGGERS_FILE_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

#include "clueapi/modules/logging/detail/base_logger/base_logger.hxx"
#include "clueapi/modules/logging/detail/hash/hash.hxx"

#include "clueapi/shared/macros.hxx"

namespace clueapi::modules::logging {
    /**
     * @struct file_logger_t
     *
     * @brief A logger that writes messages to a file.
     */
    struct file_logger_t final : detail::base_logger_t {
        /**
         * @brief Constructs a file logger with the given parameters.
         *
         * @param params The parameters for the logger.
         */
        CLUEAPI_INLINE file_logger_t(detail::logger_params_t params) noexcept
            : base_logger_t{std::move(params)} {
            m_file_hash = detail::rt_hash(m_file_path);
        }

        CLUEAPI_INLINE ~file_logger_t() noexcept {
            m_buffer.clear();

            if (m_file.is_open())
                m_file.close();
        }

       public:
        /**
         * @brief Logs a message to the file.
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

       public:
        /**
         * @brief Gets the current file path.
         *
         * @return The file path.
         */
        CLUEAPI_INLINE std::string_view file_path() const noexcept {
            return m_file_path;
        }

        /**
         * @brief Sets the file path for logging.
         *
         * @param path The new file path.
         */
        CLUEAPI_INLINE void set_file_path(std::string path) noexcept {
            m_file_path = std::move(path);
            m_file_hash = detail::rt_hash(m_file_path);
        }

       private:
        /**
         * @brief Handles changes to the file path.
         */
        void file_path_changed();

       private:
        /**
         * @brief The file stream for logging.
         */
        std::ofstream m_file;

        /**
         * @brief The previous file path hash.
         */
        std::string m_file_path{"/tmp/clueapi.log"};

        /**
         * @brief The current file path hash.
         */
        detail::hash_t m_file_hash{};

        /**
         * @brief The previous file path.
         */
        detail::hash_t m_prev_file_hash{};
    };
} // namespace clueapi::modules::logging
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_LOGGERS_FILE_HXX