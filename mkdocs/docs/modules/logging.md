# Logging Module

The `clueapi` logging module is a powerful and flexible system designed for high-performance, structured logging. It can be used as an integrated part of the `clueapi` server or as a standalone logging library in any C++ project.

---

## Features

- **Asynchronous & Synchronous Modes**: Choose between non-blocking asynchronous logging for high-throughput applications or synchronous logging for simplicity.
- **Extensible Logger Interface**: Comes with built-in `console` and `file` loggers. You can easily extend it by creating custom loggers that inherit from `base_logger_t`.
- **Compile-Time Control**: The entire module can be disabled via the `CLUEAPI_USE_LOGGING_MODULE` CMake option, resulting in zero overhead.
- **Rich Configuration**: Control log levels, buffer capacity, and batch sizes for fine-tuned performance.
- **Convenience Macros**: Use macros like `CLUEAPI_LOG_INFO` for clean and simple logging calls.

---

## Standalone Usage

You can use the logging module in any C++ application, completely independent of the `clueapi` web server.

### 1. Include and Initialize

You need to create an instance of `clueapi::modules::logging::c_logging` and configure it using its specific `cfg_t`.

```cpp
#include <clueapi/modules/logging/logging.hxx>
#include <thread>

int main() {
    // 1. Create a logging system instance
    auto logging = clueapi::modules::logging::c_logging();

    // 2. Configure it
    clueapi::modules::logging::cfg_t log_cfg{};

    {
        log_cfg.m_async_mode = true; // Use a background thread for logging
        log_cfg.m_default_level = clueapi::modules::logging::e_log_level::debug;
        log_cfg.m_sleep = std::chrono::milliseconds(10);
    }
        
    logging.init(log_cfg);

    // 3. Create and add a logger
    auto console_logger = std::make_shared<clueapi::modules::logging::console_logger_t>(
        clueapi::modules::logging::logger_params_t{
            .m_name = "my-standalone-app",
            .m_level = clueapi::modules::logging::e_log_level::info,
            .m_async_mode = true
        }
    );

    logging.add_logger(LOGGER_NAME("console"), console_logger);

    // 4. Log messages using the instance
    CLUEAPI_LOG_IMPL(
        logging,
        LOGGER_NAME("console"),
        clueapi::modules::logging::e_log_level::info,
        "Application starting..."
    );

    CLUEAPI_LOG_IMPL(
        logging,
        LOGGER_NAME("console"),
        clueapi::modules::logging::e_log_level::debug,
        "This is a debug message and will not be shown." // Wont show because logger level is info
    );

    // Give the async logger time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // The logging instance will clean up its thread in its destructor.
    return 0;
}
```

---

## Integration with clueapi

When using the main `clueapi::api()` object, a global logging instance (`clueapi::g_logging`) is automatically initialized. You configure it through the `logging_cfg_t` member of the main `clueapi::cfg_t`.

### 1. Configuration

All settings are managed within your main server configuration.

```cpp
#include <clueapi.hxx>

int main() {
    auto api = clueapi::api();
    auto cfg = clueapi::cfg_t{};

    { // Configure the global logger via the main config struct
        cfg.m_logging_cfg.m_async_mode = true;
        cfg.m_logging_cfg.m_default_level = clueapi::modules::logging::e_log_level::info;
        cfg.m_logging_cfg.m_name = "my-clueapi-server";
        cfg.m_logging_cfg.m_capacity = 8192;
        cfg.m_logging_cfg.m_batch_size = 512;
    }

    api.start(cfg);
        
    // ...
}
```

### 2. Usage

The convenience macros `CLUEAPI_LOG_DEBUG`, `CLUEAPI_LOG_INFO`, etc., automatically use the global `clueapi::g_logging` instance and the default `"clueapi"` logger.

```cpp
#include <clueapi.hxx>

clueapi::http::types::response_t get_root(clueapi::http::ctx_t ctx) {
    CLUEAPI_LOG_INFO("Handling request for URI: {}", ctx.request().uri());

    try {
        // ... some logic that might fail ...
        throw std::runtime_error("Internal logic failure");
    } catch(const std::exception& e) {
        CLUEAPI_LOG_ERROR("An error occurred during request handling: {}", e.what());
    }

    return clueapi::http::types::text_response_t{"Hello!"};
}

int main() {
    auto api = clueapi::api();
    
    auto cfg = clueapi::cfg_t{};

    api.start(cfg);

    api.add_method(clueapi::http::types::method_t::get, "/", get_root);

    CLUEAPI_LOG_INFO("Server is running!");

    api.wait();
    api.stop();

    return 0;
}
```