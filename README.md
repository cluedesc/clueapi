<h1 align="center">clueapi</h1>

<p align="center">
  <a href="https://cluedesc.github.io/clueapi/index"><img src="https://img.shields.io/badge/Mk-Docs-blue.svg" alt="MkDocs"></a>
  <a href="https://cluedesc.github.io/clueapi/doxygen"><img src="https://img.shields.io/badge/Doxygen-Docs-blue.svg" alt="DoxygenDocs"></a>
  <a href="."><img src="https://img.shields.io/badge/Version-1.0.0-blue.svg" alt="Version"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License"></a>
  <a href="https://github.com/cluedesc/clueapi/actions/workflows/tests.yml"><img src="https://github.com/cluedesc/clueapi/actions/workflows/tests.yml/badge.svg" alt="Build"></a>
</p>

<p align="center">
  <strong>A high-performance, lightweight, and modern C++20 web framework for building fast web services and APIs.</strong>
</p>

---

`clueapi` is engineered from the ground up to provide a robust and efficient platform for developing web services in C++. It leverages modern C++20 features, including coroutines, and is built upon the solid foundation of Boost.Asio to deliver a fully asynchronous, high-performance, and scalable solution. It's designed for developers who require low-level control and high throughput without sacrificing clean, maintainable code.

## Key Features

`clueapi` is more than just a simple web server; it's a comprehensive framework packed with advanced features for professional development.

* **Modern Asynchronous Core**: At its heart, `clueapi` uses C++20 coroutines (`co_await`) and a Boost.Asio `io_context` for all I/O operations. This enables a fully asynchronous, non-blocking architecture capable of handling tens of thousands of concurrent connections efficiently.

* **Efficient Radix Tree Routing**: A highly-optimized radix tree implementation provides fast and flexible request routing:
    * **Dynamic & Static Routes**: Supports static paths (`/api/v1/health`), parameterized segments (`/users/{id}`), and complex nested routes.
    * **Method-Based Dispatch**: Correctly dispatches requests to different handlers for the same URL path based on the HTTP method (GET, POST, etc.).
    * **Conflict Detection**: Throws exceptions at startup for ambiguous routes (e.g., `/users/{id}` and `/users/{uuid}`), preventing runtime errors.

* **Expressive Middleware Chain**: Implement cross-cutting concerns with a clean and powerful middleware system. Each middleware component can inspect or modify requests and responses, or short-circuit the chain entirely.

* **Advanced Multipart/Form-Data Handling**: The built-in multipart parser is designed for performance and large file uploads:
    * **Streaming Parser**: It can parse directly from a request body in memory or stream from a temporary file on disk.
    * **Automatic Disk Offloading**: To conserve memory, the parser will automatically spill file uploads to disk when they exceed a configurable size threshold.

* **Streaming Responses (Chunked Encoding)**: Easily send large responses without buffering the entire content in memory. `clueapi` provides a utility for sending data using HTTP chunked transfer encoding, ideal for streaming large files or real-time data.

* **Modular & Configurable**:
    * **Optional Modules**: Enable or disable features like **Logging** and **Dotenv** support at compile time to create a lean build tailored to your needs.
    * **Extensive Configuration**: A single struct provides centralized control over hundreds of parameters, from server worker counts to low-level socket options.
 
## Quick Start

The following example demonstrates a simple API with a static and dynamic routes that returns a TEXT response.

```cpp
#include <clueapi.hxx>

clueapi::http::types::response_t root_sync(clueapi::http::ctx_t ctx) {
    // first option for initializing the response
    clueapi::http::types::text_response_t resp{};

    {
        resp.body()   = "Hello, world!";
        resp.status() = clueapi::http::types::status_t::e_status::ok;
    }

    return resp;
}

clueapi::shared::awaitable_t<clueapi::http::types::response_t> root_async(clueapi::http::ctx_t ctx) {
    // second option for initializing the response
    co_return clueapi::http::types::text_response_t{
        "Hello, world!",
        clueapi::http::types::status_t::e_status::ok,
    };
}

clueapi::http::types::response_t get_user(clueapi::http::ctx_t ctx) {
    std::vector<std::string> users{
        "user1",
        "user2",
        "user3",
    };

    clueapi::http::types::text_response_t resp{};

    // Extract the 'user_id' parameter from the URL
    const auto& user_id = std::stoi(ctx.params()["user_id"]);

    if (users.size() < user_id) {
        resp.body()   = "User not found";
        resp.status() = clueapi::http::types::status_t::e_status::not_found;

        return resp;
    }

    {
        resp.body()   = users.at(user_id);
        resp.status() = clueapi::http::types::status_t::e_status::ok;
    }

    return resp;
}

int main() {
    auto api = clueapi::api();

    auto cfg = clueapi::cfg_t{};

    {
        cfg.m_host = "localhost";
        cfg.m_port = "8080";

        cfg.m_server.m_workers = 4;
    }

    {
        api.add_method(
            clueapi::http::types::method_t::e_method::get,

            "/sync",

            root_sync
        );

        api.add_method(
            clueapi::http::types::method_t::e_method::get,

            "/async",

            root_async
        );

        api.add_method(
            clueapi::http::types::method_t::e_method::get,

            "/user/{user_id}",

            get_user
        );
    }

    api.start(cfg);

    { // block until the server is stopped
        api.wait();
    }

    api.stop();

    return 0;
}
```

## Dependencies

* **C++20** compatible compiler (GCC, Clang, MSVC, Intel)
* **CMake** (>= 3.20)
* **Boost** (>= 1.84.0) with components: `system`, `filesystem`, `iostreams`
* **OpenSSL**
* **liburing** (Optional, for Linux `io_uring` support)

The following dependencies are included as submodules:

* `fmt`
* `nlohmann/json`
* `ankerl/unordered_dense`
* `TartanLlama/expected`

## Installation

1.  **Clone the repository with submodules:**
    ```bash
    git clone --recurse-submodules https://github.com/your-username/clueapi.git
    cd clueapi
    ```

2.  **Configure and build with CMake:**
    ```bash
    # Create a build directory
    cmake -B build
    
    # Build the project
    cmake --build build -j $(nproc) 
    ```

3.  **Run tests (optional):**
    ```bash
    cd build
    ctest
    ```

4.  **Install (optional):**
    ```bash
    sudo cmake --install build
    ```

---

### Build Options

You can customize the build using the following CMake options:

| Option                       | Description                               | Default |
| ---------------------------- | ----------------------------------------- | ------- |
| `CLUEAPI_USE_NLOHMANN_JSON`  | Enable nlohmann/json support              | `ON`    |
| `CLUEAPI_USE_LOGGING_MODULE` | Enable the logging module                 | `ON`    |
| `CLUEAPI_USE_DOTENV_MODULE`  | Enable the dotenv module                  | `ON`    |
| `CLUEAPI_USE_RTTI`           | Enable Run-Time Type Information          | `OFF`   |
| `CLUEAPI_USE_SANITIZERS`     | Enable Address/Leak sanitizers            | `OFF`   |
| `CLUEAPI_RUN_TESTS`          | Build and enable tests                    | `ON`    |

## Benchmark

Code

```cpp
#include <clueapi.hxx>

clueapi::shared::awaitable_t<clueapi::http::types::response_t> root_async(clueapi::http::ctx_t ctx) {
    co_return clueapi::http::types::text_response_t{
        "Hello, world!",
        clueapi::http::types::status_t::e_status::ok,
    };
}

int main() {
    auto api = clueapi::api();

    auto cfg = clueapi::cfg_t{};

    {
        cfg.m_host = "localhost";
        cfg.m_port = "8080";

        cfg.m_server.m_workers = 8;
    }

    {
        api.add_method(
            clueapi::http::types::method_t::e_method::get,

            "/root",

            root_async
        );
    }

    api.start(cfg);

    {
        api.wait();
    }

    api.stop();

    return 0;
}
```

Server config

```
CPU: AMD Ryzen 5 7535U
RAM: 16gb DDR5

OS: Ubuntu-24.04
```

Command

```bash
wrk -t4 -c100 -d30s http://localhost:8080/root
```

Result

```
Running 30s test @ http://localhost:8080/root
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   265.53us   80.65us   9.04ms   91.51%
    Req/Sec    87.73k     4.35k  146.72k    73.02%
  10483178 requests in 30.10s, 0.95GB read
Requests/sec: 348284.47
Transfer/sec:     32.22MB
```

## Roadmap

Future development will focus on expanding the ecosystem and adding more enterprise-grade features:

* **OpenAPI Integration**: Automatic generation of OpenAPI (Swagger) specifications from route definitions for seamless API documentation and client generation.
* **Database Modules**: Dedicated modules for interacting with popular databases like **PostgreSQL** and **Redis**, providing connection pooling and asynchronous query execution.
* **Metrics & Observability**: A module to expose application metrics in the **Prometheus** format for easy integration into modern monitoring and alerting pipelines.
* **WebSocket Support**: First-class support for WebSocket connections to enable real-time, bidirectional communication.

## Contributing

Contributions are highly welcome. Please feel free to open an issue to discuss your ideas or submit a pull request with your improvements.

## License

This project is licensed under the **MIT License**.