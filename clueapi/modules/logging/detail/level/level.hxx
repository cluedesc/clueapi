/**
 * @file level.hxx
 *
 * @brief Defines the log level enumeration and utility functions.
 */

#ifndef CLUEAPI_MODULES_LOGGING_DETAIL_LEVEL_HXX
#define CLUEAPI_MODULES_LOGGING_DETAIL_LEVEL_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include <cstdint>
#include <chrono>
#include <string_view>
#include <string>

#include "clueapi/modules/logging/detail/level/level.hxx"

#include "clueapi/shared/macros.hxx"

namespace clueapi::modules::logging::detail {
    /**
     * @enum e_log_level
     *
     * @brief Defines the different levels of logging.
     *
     * @internal
     */
    enum struct e_log_level : std::uint8_t { trace, debug, info, warning, error, critical, off };

    /**
     * @brief Converts a log level to its string representation.
     *
     * @param level The log level to convert.
     *
     * @return The string representation of the log level.
     *
     * @internal
     */
    CLUEAPI_INLINE std::string_view lvl_to_str(e_log_level level) noexcept {
        switch (level) {
            case e_log_level::trace:
                return "TRACE";

            case e_log_level::debug:
                return "DEBUG";

            case e_log_level::info:
                return "INFO";

            case e_log_level::warning:
                return "WARNING";

            case e_log_level::error:
                return "ERROR";

            case e_log_level::critical:
                return "CRITICAL";

            default:
                return "UNKNOWN";
        }
    }
} // namespace clueapi::modules::logging::detail
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_DETAIL_LEVEL_HXX