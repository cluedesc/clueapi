# Dotenv Module

The `dotenv` module provides a simple and effective way to manage your application's configuration by loading environment variables from a `.env` file. This practice helps separate configuration from code, which is essential for security and maintainability.

---

## Features

- **Zero-Dependency Loading**: Parses standard `.env` files.
- **Type-Safe Access**: Retrieve variables as `std::string`, `int`, `bool`, etc., with support for default values.
- **High-Performance Lookups**: Uses compile-time or runtime hashing of keys (`ENV_NAME`, `ENV_NAME_RT`) for fast access.
- **Compile-Time Control**: Can be excluded from the build via the `CLUEAPI_USE_DOTENV_MODULE` CMake flag.
- **Convenience Features**: Ignores comments and empty lines, and can optionally trim whitespace from values.

---

## Standalone Usage

You can use the `dotenv` module to manage configuration in any C++ application.

### 1. Create a `.env` file

```env
# Database Settings
DB_HOST=127.0.0.1
DB_PORT=5432
ENABLE_SSL=true
```

### 2. Load and Use

Create an instance of `clueapi::modules::dotenv::c_dotenv` and call the `load()` method.

```cpp
#include <clueapi/modules/dotenv/dotenv.hxx>
#include <iostream>

int main() {
    // 1. Create a dotenv instance
    auto dotenv = clueapi::modules::dotenv::c_dotenv();

    // 2. Load the .env file (and trim whitespace from values)
    dotenv.load(".env", true);

    // 3. Retrieve values using the instance
    auto host = dotenv.at<std::string>(ENV_NAME("DB_HOST"), "localhost");
    auto port = dotenv.at<int>(ENV_NAME("DB_PORT"));
    auto use_ssl = dotenv.at<bool>(ENV_NAME("ENABLE_SSL"), false);

    std::cout << "Connecting to " << host << ":" << port;

    if (use_ssl) {
        std::cout << " with SSL." << std::endl;
    } else {
        std::cout << " without SSL." << std::endl;
    }

    return 0;
}
```

---

## Integration with clueapi

When using `clueapi`, a global instance `clueapi::g_dotenv` is made available. You are responsible for loading your `.env` file at the start of your application. The framework provides the instance and convenience macros for easy access throughout your project.

### 1. Create a `.env` file

```env
# Server Configuration
SERVER_HOST=0.0.0.0
SERVER_PORT=8080
WORKER_THREADS=4
```

### 2. Load and Use in `main()`

Load the `.env` file using the global `g_dotenv` instance before you configure and start your server.

```cpp
#include <clueapi.hxx>

int main() {
    // 1. Load the .env file using the global instance
    clueapi::g_dotenv->load(".env");

    // 2. Use the convenience macro to get values
    auto port = CLUEAPI_DOTENV_GET("SERVER_PORT", std::string, "8000");
    auto workers = CLUEAPI_DOTENV_GET("WORKER_THREADS", int, 2);

    auto api = clueapi::api();
    
    auto cfg = clueapi::cfg_t{};

    { // 3. Use the loaded values to configure the server
        cfg.m_port = port;
        cfg.m_server.m_workers = workers;
    }
        
    CLUEAPI_LOG_INFO("Configuring server with {} workers on port {}", workers, port);

    api.start(cfg);

    api.wait();

    api.stop();

    return 0;
}
```