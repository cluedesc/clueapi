# Redis Module

The redis module provides a high-performance, fully asynchronous client for interacting with a Redis server, built on top of Boost Redis. It is designed to provide an efficient and convenient way to integrate Redis into your applications as a cache, message broker, or data store.

**Currently support only coroutines.**

---

## Features

- **Fully Asynchronous Architecture**: All Redis operations (e.g., GET, SET, HSET) are performed asynchronously using C++20 coroutines (co_await), ensuring high throughput and minimal thread usage.
- **Automatic Reconnection**: The client automatically manages the connection lifecycle, including reconnecting in case of failures and performing periodic health checks to ensure reliability.
- **Connection Safe Management**: Connections are thread-safe and managed via shared_ptr, simplifying their use in multi-threaded applications.
- **Flexible Configuration**: Provides detailed configuration options, including timeouts, authentication credentials, and SSL/TLS settings.
- **Compile-Time Control**: The module can be completely excluded from the build using the CLUEAPI_USE_REDIS_MODULE CMake flag.

---

## Usage

The redis module can be used to create and manage connections to Redis, execute commands, and handle responses asynchronously.

```cpp
#include <clueapi/clueapi.hxx>

clueapi::modules::redis::c_redis redis_module{};

clueapi::shared::awaitable_t<clueapi::http::types::response_t> root_async(clueapi::http::ctx_t ctx) {
    auto connection = redis_module.create_connection();

    {
        auto is_connected = co_await connection->connect();

        if (!is_connected) {
            co_return clueapi::http::types::text_response_t{
                "Failed to connect to Redis",
                clueapi::http::types::status_t::internal_server_error
            };
        }
    }

    auto key = "test-key";

    auto key_exists = co_await connection->exists(key);

    if (key_exists) {
        auto key_del = co_await connection->del(key);

        if (!key_del) {
            co_return clueapi::http::types::text_response_t{
                "Failed to delete key",
                clueapi::http::types::status_t::internal_server_error
            };
        }
    }

    std::string value = "Hello, world!";

    auto key_set = co_await connection->set(key, value);

    if (!key_set) {
        co_return clueapi::http::types::text_response_t{
            "Failed to set key",
            clueapi::http::types::status_t::internal_server_error
        };
    }

    auto key_get = co_await connection->get<std::string>(key);

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