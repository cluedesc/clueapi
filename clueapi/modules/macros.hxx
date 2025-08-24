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
#define CLUEAPI_DOTENV_GET(key, type, default_val) \
    CLUEAPI_DOTENV_GET_IMPL(clueapi::g_dotenv.get(), ENV_NAME(key), type, default_val)
#else
#define CLUEAPI_DOTENV_GET(key, type, default_val) default_val
#endif // CLUEAPI_USE_DOTENV_MODULE

#ifdef CLUEAPI_USE_LOGGING_MODULE
#define CLUEAPI_LOG_LEVEL_TRACE 0
#define CLUEAPI_LOG_LEVEL_DEBUG 1
#define CLUEAPI_LOG_LEVEL_INFO 2
#define CLUEAPI_LOG_LEVEL_WARNING 3
#define CLUEAPI_LOG_LEVEL_ERROR 4
#define CLUEAPI_LOG_LEVEL_CRITICAL 5
#define CLUEAPI_LOG_LEVEL_NONE 6

#if defined(CLUEAPI_OPTIMIZED_LOG_LEVEL)
#if CLUEAPI_OPTIMIZED_LOG_LEVEL == CLUEAPI_LOG_LEVEL_TRACE
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_TRACE
#elif CLUEAPI_OPTIMIZED_LOG_LEVEL == CLUEAPI_LOG_LEVEL_DEBUG
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_DEBUG
#elif CLUEAPI_OPTIMIZED_LOG_LEVEL == CLUEAPI_LOG_LEVEL_INFO
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_INFO
#elif CLUEAPI_OPTIMIZED_LOG_LEVEL == CLUEAPI_LOG_LEVEL_WARNING
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_WARNING
#elif CLUEAPI_OPTIMIZED_LOG_LEVEL == CLUEAPI_LOG_LEVEL_ERROR
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_ERROR
#elif CLUEAPI_OPTIMIZED_LOG_LEVEL == CLUEAPI_LOG_LEVEL_CRITICAL
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_CRITICAL
#elif CLUEAPI_OPTIMIZED_LOG_LEVEL == CLUEAPI_LOG_LEVEL_NONE
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_NONE
#else
#error "Unknown CLUEAPI_OPTIMIZED_LOG_LEVEL"
#endif
#else
#define CLUEAPI_OPT_LOG_NUM CLUEAPI_LOG_LEVEL_NONE
#endif

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
#define CLUEAPI_LOG(hash, level, message, ...) \
    CLUEAPI_LOG_IMPL(clueapi::g_logging.get(), hash, level, message, ##__VA_ARGS__)

/**
 * @def CLUEAPI_LOG_TRACE
 *
 * @brief A convenience macro for logging a trace message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#if CLUEAPI_OPT_LOG_NUM <= CLUEAPI_LOG_LEVEL_TRACE
#define CLUEAPI_LOG_TRACE(message, ...)                \
    CLUEAPI_LOG(                                       \
        LOGGER_NAME("clueapi"),                        \
        clueapi::modules::logging::e_log_level::trace, \
        message,                                       \
        ##__VA_ARGS__)
#else
#define CLUEAPI_LOG_TRACE(message, ...) \
    do {                                \
    } while (0)
#endif

/**
 * @def CLUEAPI_LOG_DEBUG
 *
 * @brief A convenience macro for logging a debug message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#if CLUEAPI_OPT_LOG_NUM <= CLUEAPI_LOG_LEVEL_DEBUG
#define CLUEAPI_LOG_DEBUG(message, ...)                \
    CLUEAPI_LOG(                                       \
        LOGGER_NAME("clueapi"),                        \
        clueapi::modules::logging::e_log_level::debug, \
        message,                                       \
        ##__VA_ARGS__)
#else
#define CLUEAPI_LOG_DEBUG(message, ...) \
    do {                                \
    } while (0)
#endif

/**
 * @def CLUEAPI_LOG_INFO
 *
 * @brief A convenience macro for logging an info message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#if CLUEAPI_OPT_LOG_NUM <= CLUEAPI_LOG_LEVEL_INFO
#define CLUEAPI_LOG_INFO(message, ...)                \
    CLUEAPI_LOG(                                      \
        LOGGER_NAME("clueapi"),                       \
        clueapi::modules::logging::e_log_level::info, \
        message,                                      \
        ##__VA_ARGS__)
#else
#define CLUEAPI_LOG_INFO(message, ...) \
    do {                               \
    } while (0)
#endif

/**
 * @def CLUEAPI_LOG_WARNING
 *
 * @brief A convenience macro for logging a warning message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#if CLUEAPI_OPT_LOG_NUM <= CLUEAPI_LOG_LEVEL_WARNING
#define CLUEAPI_LOG_WARNING(message, ...)                \
    CLUEAPI_LOG(                                         \
        LOGGER_NAME("clueapi"),                          \
        clueapi::modules::logging::e_log_level::warning, \
        message,                                         \
        ##__VA_ARGS__)
#else
#define CLUEAPI_LOG_WARNING(message, ...) \
    do {                                  \
    } while (0)
#endif

/**
 * @def CLUEAPI_LOG_ERROR
 *
 * @brief A convenience macro for logging an error message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#if CLUEAPI_OPT_LOG_NUM <= CLUEAPI_LOG_LEVEL_ERROR
#define CLUEAPI_LOG_ERROR(message, ...)                \
    CLUEAPI_LOG(                                       \
        LOGGER_NAME("clueapi"),                        \
        clueapi::modules::logging::e_log_level::error, \
        message,                                       \
        ##__VA_ARGS__)
#else
#define CLUEAPI_LOG_ERROR(message, ...) \
    do {                                \
    } while (0)
#endif

/**
 * @def CLUEAPI_LOG_CRITICAL
 *
 * @brief A convenience macro for logging a critical message.
 *
 * @param message The format string for the log message.
 * @param ... The format arguments.
 */
#if CLUEAPI_OPT_LOG_NUM <= CLUEAPI_LOG_LEVEL_CRITICAL
#define CLUEAPI_LOG_CRITICAL(message, ...)                \
    CLUEAPI_LOG(                                          \
        LOGGER_NAME("clueapi"),                           \
        clueapi::modules::logging::e_log_level::critical, \
        message,                                          \
        ##__VA_ARGS__)
#else
#define CLUEAPI_LOG_CRITICAL(message, ...) \
    do {                                   \
    } while (0)
#endif
#else
#define CLUEAPI_LOG(hash, level, message, ...)

#define CLUEAPI_LOG_DEBUG(message, ...)

#define CLUEAPI_LOG_INFO(message, ...)

#define CLUEAPI_LOG_WARNING(message, ...)

#define CLUEAPI_LOG_ERROR(message, ...)

#define CLUEAPI_LOG_CRITICAL(message, ...)
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_MACROS_HXX