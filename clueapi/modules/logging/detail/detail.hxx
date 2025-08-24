/**
 * @file detail.hxx
 *
 * @brief Includes all internal detail headers for the logging module.
 */

#ifndef CLUEAPI_MODULES_LOGGING_DETAIL_HXX
#define CLUEAPI_MODULES_LOGGING_DETAIL_HXX

#include "hash/hash.hxx"

#include "level/level.hxx"

#include "log/log.hxx"

#include "buffer/buffer.hxx"

#include "base_logger/base_logger.hxx"

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging::detail {
    /**
     * @brief The size of the memory buffer for batch formatting.
     *
     * @internal
     */
    inline constexpr auto k_memory_buffer_size = 16192;

    /**
     * @brief A type alias for a batch memory buffer.
     *
     * @internal
     */
    using batch_memory_buffer_t = fmt::basic_memory_buffer<char, k_memory_buffer_size>;

    /**
     * @brief Prints the contents of a batch memory buffer to the console.
     *
     * @param buffer The buffer to print.
     *
     * @internal
     */
    CLUEAPI_INLINE void print(batch_memory_buffer_t& buffer) noexcept {
        try {
            fmt::print(FMT_COMPILE("{}"), std::string_view(buffer.data(), buffer.size()));

            buffer.clear();
        } catch (...) {
            // ...
        }
    }

    /**
     * @brief Prints the contents of a memory buffer to the console.
     *
     * @param buffer The buffer to print.
     *
     * @internal
     */
    CLUEAPI_INLINE void print(fmt::memory_buffer& buffer) noexcept {
        try {
            fmt::print(FMT_COMPILE("{}"), std::string_view(buffer.data(), buffer.size()));

            buffer.clear();
        } catch (...) {
            // ...
        }
    }

    /**
     * @brief Prints the contents of a batch memory buffer to a file.
     *
     * @param file The file stream to write to.
     * @param buffer The buffer to print.
     * @param flush Whether to flush the file stream after writing.
     *
     * @internal
     */
    CLUEAPI_INLINE void print(
        std::ofstream& file, batch_memory_buffer_t& buffer, bool flush = true) noexcept {
        file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        if (flush)
            file.flush();

        buffer.clear();
    }

    /**
     * @brief Prints the contents of a memory buffer to a file.
     *
     * @param file The file stream to write to.
     * @param buffer The buffer to print.
     * @param flush Whether to flush the file stream after writing.
     *
     * @internal
     */
    CLUEAPI_INLINE void print(
        std::ofstream& file, fmt::memory_buffer& buffer, bool flush = true) noexcept {
        file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        if (flush)
            file.flush();

        buffer.clear();
    }

    /**
     * @brief Formats a log message into a batch memory buffer.
     *
     * @param msg The log message to format.
     * @param buffer The buffer to format into.
     *
     * @return `true` on success, `false` otherwise.
     *
     * @internal
     */
    CLUEAPI_INLINE bool format(const log_msg_t& msg, batch_memory_buffer_t& buffer) noexcept {
        try {
            auto in_time_t = std::chrono::system_clock::to_time_t(msg.m_time);

            if (!in_time_t)
                in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

            fmt::format_to(
                std::back_inserter(buffer),

                FMT_COMPILE("[{:%Y-%m-%d %H:%M:%S}] {} - {}\n"),

                fmt::styled(
                    *std::localtime(&in_time_t), fmt::emphasis::bold | fg(fmt::rgb(245, 245, 184))),

                fmt::styled(lvl_to_str(msg.m_level), fmt::emphasis::bold),

                fmt::styled(msg.m_msg, fg(fmt::rgb(255, 255, 230))));

            return true;
        } catch (...) {
            // ...
        }

        return false;
    }

    /**
     * @brief Formats a log message into a memory buffer.
     *
     * @param msg The log message to format.
     * @param buffer The buffer to format into.
     *
     * @return `true` on success, `false` otherwise.
     *
     * @internal
     */
    CLUEAPI_INLINE bool format(const log_msg_t& msg, fmt::memory_buffer& buffer) noexcept {
        try {
            auto in_time_t = std::chrono::system_clock::to_time_t(msg.m_time);

            if (!in_time_t)
                in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

            fmt::format_to(
                std::back_inserter(buffer),

                FMT_COMPILE("[{:%Y-%m-%d %H:%M:%S}] {} - {}\n"),

                fmt::styled(
                    *std::localtime(&in_time_t), fmt::emphasis::bold | fg(fmt::rgb(245, 245, 184))),

                fmt::styled(lvl_to_str(msg.m_level), fmt::emphasis::bold),

                fmt::styled(msg.m_msg, fg(fmt::rgb(255, 255, 230))));

            return true;
        } catch (...) {
            // ...
        }

        return false;
    }
} // namespace clueapi::modules::logging::detail
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_DETAIL_HXX