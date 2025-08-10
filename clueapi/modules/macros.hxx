/**
 * @file macros.hxx
 *
 * @brief Defines convenience macros for accessing modules.
 */

#ifndef CLUEAPI_MODULES_MACROS_HXX
#define CLUEAPI_MODULES_MACROS_HXX

#ifdef CLUEAPI_USE_DOTENV_MODULE
/**
 * @def CLUEAPI_DOTENV_GET
 *
 * @brief A convenience macro for retrieving a value from the global dotenv instance.
 *
 * @param key The key of the variable to retrieve.
 * @param type The desired type of the value.
 * @param default_val The default value to return if the key is not found.
 */
#define CLUEAPI_DOTENV_GET(key, type, default_val)                                                                     \
    CLUEAPI_DOTENV_GET_IMPL(clueapi::g_dotenv.get(), ENV_NAME(key), type, default_val)
#else
#define CLUEAPI_DOTENV_GET(key, type, default_val) default_val
#endif // CLUEAPI_USE_DOTENV_MODULE

#ifdef CLUEAPI_USE_LOGGING_MODULE
/**
 * @def CLUEAPI_LOG
 *
 * @brief A convenience macro for logging a message with a specific logger.
 *
 * @param hash The hash of the logger's name.
 * @param level The log level.
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#define CLUEAPI_LOG(hash, level, message, ...)                                                                         \
    CLUEAPI_LOG_IMPL(clueapi::g_logging.get(), hash, level, message, ##__VA_ARGS__)

/**
 * @def CLUEAPI_LOG_DEBUG
 *
 * @brief A convenience macro for logging a debug message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#define CLUEAPI_LOG_DEBUG(message, ...)                                                                                \
    CLUEAPI_LOG(LOGGER_NAME("clueapi"), clueapi::modules::logging::e_log_level::debug, message, ##__VA_ARGS__)

/**
 * @def CLUEAPI_LOG_INFO
 *
 * @brief A convenience macro for logging an info message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#define CLUEAPI_LOG_INFO(message, ...)                                                                                 \
    CLUEAPI_LOG(LOGGER_NAME("clueapi"), clueapi::modules::logging::e_log_level::info, message, ##__VA_ARGS__)

/**
 * @def CLUEAPI_LOG_WARNING
 *
 * @brief A convenience macro for logging a warning message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#define CLUEAPI_LOG_WARNING(message, ...)                                                                              \
    CLUEAPI_LOG(LOGGER_NAME("clueapi"), clueapi::modules::logging::e_log_level::warning, message, ##__VA_ARGS__)

/**
 * @def CLUEAPI_LOG_ERROR
 *
 * @brief A convenience macro for logging an error message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#define CLUEAPI_LOG_ERROR(message, ...)                                                                                \
    CLUEAPI_LOG(LOGGER_NAME("clueapi"), clueapi::modules::logging::e_log_level::error, message, ##__VA_ARGS__)

/**
 * @def CLUEAPI_LOG_CRITICAL
 *
 * @brief A convenience macro for logging a critical message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#define CLUEAPI_LOG_CRITICAL(message, ...)                                                                             \
    CLUEAPI_LOG(LOGGER_NAME("clueapi"), clueapi::modules::logging::e_log_level::critical, message, ##__VA_ARGS__)
#else
#define CLUEAPI_LOG(hash, level, message, ...)

#define CLUEAPI_LOG_DEBUG(message, ...)

#define CLUEAPI_LOG_INFO(message, ...)

#define CLUEAPI_LOG_WARNING(message, ...)

#define CLUEAPI_LOG_ERROR(message, ...)

#define CLUEAPI_LOG_CRITICAL(message, ...)
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_MACROS_HXX