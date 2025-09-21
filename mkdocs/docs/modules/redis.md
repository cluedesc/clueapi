# Redis Module

The redis module provides a high-performance, dual-API client for interacting with a Redis server, built on top of Boost.Redis. It is designed to offer both fully asynchronous (coroutine-based) and convenient synchronous (blocking) interfaces to integrate Redis into your applications.

---

## Features

- **Dual Asynchronous and Synchronous API**: All Redis operations (e.g., GET, SET, HSET) are available in two flavors: asynchronous (`async_*`) using C++20 coroutines for maximum performance, and synchronous (`sync_*`) for simpler, blocking execution in non-asynchronous contexts.
- **Automatic Reconnection**: The client automatically manages the connection lifecycle, including reconnecting in case of failures and performing periodic health checks to ensure reliability.
- **Thread-Safe Connections**: Connections are managed via `std::shared_ptr`, simplifying their use in multi-threaded applications. The internal state is handled atomically.
- **Flexible Configuration**: Provides detailed configuration options, including timeouts, authentication credentials, and SSL/TLS settings.
- **Compile-Time Control**: The module can be completely excluded from the build using the `CLUEAPI_USE_REDIS_MODULE` CMake flag.

---

## Usage

The redis module can be used to create and manage connections to Redis, execute commands, and handle responses asynchronously or synchronously.

```cpp
#include <clueapi/clueapi.hxx>

clueapi::modules::redis::c_redis redis_module{};

clueapi::shared::awaitable_t<clueapi::http::types::response_t> root_async(clueapi::http::ctx_t ctx) {
    auto connection = redis_module.create_connection();

    {
        auto is_connected = co_await connection->async_connect();

        if (!is_connected) {
            co_return clueapi::http::types::text_response_t{
                "Failed to connect to Redis",
                clueapi::http::types::status_t::internal_server_error
            };
        }
    }

    auto key = "test-key";

    auto key_exists = co_await connection->async_exists(key);

    if (key_exists) {
        auto key_del = co_await connection->async_del(key);

        if (!key_del) {
            co_return clueapi::http::types::text_response_t{
                "Failed to delete key",
                clueapi::http::types::status_t::internal_server_error
            };
        }
    }

    std::string value = "Hello, world!";

    auto key_set = co_await connection->async_set(key, value);

    if (!key_set) {
        co_return clueapi::http::types::text_response_t{
            "Failed to set key",
            clueapi::http::types::status_t::internal_server_error
        };
    }

    auto key_get = co_await connection->async_get<std::string>(key);

    if (!key_get.has_value()) {
        co_return clueapi::http::types::text_response_t{
            "Failed to get key",
            clueapi::http::types::status_t::internal_server_error
        };
    }

    co_return clueapi::http::types::text_response_t{
        key_get.value(),
        clueapi::http::types::status_t::ok
    };
}

clueapi::http::types::response_t root_sync(clueapi::http::ctx_t ctx) {
    auto connection = redis_module.create_connection();

    {
        auto is_connected = connection->sync_connect();

        if (!is_connected) {
            return clueapi::http::types::text_response_t{
                "Failed to connect to Redis",
                clueapi::http::types::status_t::internal_server_error
            };
        }
    }

    auto key = "test-key";

    auto key_exists = connection->sync_exists(key);

    if (key_exists) {
        auto key_del = connection->sync_del(key);

        if (!key_del) {
            return clueapi::http::types::text_response_t{
                "Failed to delete key",
                clueapi::http::types::status_t::internal_server_error
            };
        }
    }

    std::string value = "Hello, world!";

    auto key_set = connection->sync_set(key, value);

    if (!key_set) {
        return clueapi::http::types::text_response_t{
            "Failed to set key",
            clueapi::http::types::status_t::internal_server_error
        };
    }

    auto key_get = connection->sync_get<std::string>(key);

    if (!key_get.has_value()) {
        return clueapi::http::types::text_response_t{
            "Failed to get key",
            clueapi::http::types::status_t::internal_server_error
        };
    }

    return clueapi::http::types::text_response_t{
        key_get.value(),
        clueapi::http::types::status_t::ok
    };
}

int main() {
    // 1. Create a Redis module instance
    auto redis_module = clueapi::modules::redis::c_redis();

    // 2. Configure the connection settings
    clueapi::modules::redis::cfg_t redis_cfg{};

    redis_cfg.m_host = "127.0.0.1";
    redis_cfg.m_port = "6379";

    // 3. Initialize the module, passing an io_context from clueapi
    auto api = clueapi::api();

    clueapi::cfg_t api_cfg{};

    api.start(api_cfg);

    redis_module.init(redis_cfg, api.io_ctx_pool().def_io_ctx());

    // ... further logic ...

    api.stop();

    return 0;
}
```