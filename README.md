<h1 align="center">clueapi</h1>

<p align="center">
  <a href="https://cluedesc.github.io/clueapi/index"><img src="https://img.shields.io/badge/Mk-Docs-blue.svg" alt="MkDocs"></a>
  <a href="https://cluedesc.github.io/clueapi/doxygen"><img src="https://img.shields.io/badge/Doxygen-Docs-blue.svg" alt="DoxygenDocs"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="C++ Standard">
  <a href="."><img src="https://img.shields.io/badge/Version-2.2.0-blue.svg" alt="Version"></a>
  <a href="https://github.com/cluedesc/clueapi/actions/workflows/tests.yml"><img src="https://github.com/cluedesc/clueapi/actions/workflows/tests.yml/badge.svg" alt="Build"></a>
  <a href="https://app.codacy.com/gh/cluedesc/clueapi/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade" target="_blank" rel="noopener noreferrer"><img src="https://app.codacy.com/project/badge/Grade/5a8d305ab93c4c9da65b0ccfb59e6d3c" alt="Codacy Grade" /></a>
</p>

<p align="center">
  <a href="https://github.com/cluedesc/clueapi/commits"><img src="https://img.shields.io/github/last-commit/cluedesc/clueapi.svg" alt="Last Commit"></a>
  <a href="https://github.com/cluedesc/clueapi/issues"><img src="https://img.shields.io/github/issues/cluedesc/clueapi.svg" alt="GitHub Issues"></a>
  <img src="https://img.shields.io/badge/platform-Linux-green.svg" alt="Platform Support">
    <img src="https://img.shields.io/badge/platform-Windows-green.svg" alt="Platform Support">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License"></a>
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
        resp.status() = clueapi::http::types::status_t::ok;
    }

    return resp;
}

clueapi::shared::awaitable_t<clueapi::http::types::response_t> root_async(clueapi::http::ctx_t ctx) {
    // second option for initializing the response
    co_return clueapi::http::types::text_response_t{
        "Hello, world!",
        clueapi::http::types::status_t::ok
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
        resp.status() = clueapi::http::types::status_t::not_found;

        return resp;
    }

    {
        resp.body()   = users.at(user_id);
        resp.status() = clueapi::http::types::status_t::ok;
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
            clueapi::http::types::method_t::get,

            "/sync",

            root_sync
        );

        api.add_method(
            clueapi::http::types::method_t::get,

            "/async",

            root_async
        );

        api.add_method(
            clueapi::http::types::method_t::get,

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
* **liburing** (For Linux)

The following dependencies are included as submodules:

* `fmt`
* `nlohmann/json`
* `ankerl/unordered_dense`
* `TartanLlama/expected`

# Installation

## Linux

1. **Clone the repository with submodules:**
```bash
git clone --recurse-submodules https://github.com/cluedesc/clueapi.git
cd clueapi
```

2. **(Optional) Remove unnecessary files from submodules:**
```bash
for m in clueapi/shared/thirdparty/fmt \
         clueapi/shared/thirdparty/nlohmann_json \
         clueapi/shared/thirdparty/ankerl_unordered_dense \
         clueapi/shared/thirdparty/tl-expected; do

    cd "$m"
    git sparse-checkout init --cone

    if [[ "$m" == *nlohmann_json ]]; then
        git sparse-checkout set single_include
    else
        git sparse-checkout set include
    fi

    find . -mindepth 1 -maxdepth 1 ! -name $( [[ "$m" == *nlohmann_json ]] && echo "single_include" || echo "include" ) -exec rm -rf {} +

    cd - >/dev/null
done
```

3. **Build using CMake:**
```bash
cmake -B build
cmake --build build -j $(nproc)
```

4. **(Optional) Run tests:**
```bash
cd build
ctest
```

5. **(Optional) Install:**
```bash
sudo cmake --install build
```

---

## Windows (PowerShell)

1. **Clone the repository with submodules:**
```powershell
git clone --recurse-submodules https://github.com/cluedesc/clueapi.git
cd clueapi
```

2. **(Optional) Remove unnecessary files from submodules:**
```powershell
$submodules = @(
    "clueapi/shared/thirdparty/fmt",
    "clueapi/shared/thirdparty/nlohmann_json",
    "clueapi/shared/thirdparty/ankerl_unordered_dense",
    "clueapi/shared/thirdparty/tl-expected"
)

foreach ($m in $submodules) {
    Set-Location $m
    git sparse-checkout init --cone

    if ($m -like "*nlohmann_json") {
        git sparse-checkout set single_include
    } else {
        git sparse-checkout set include
    }

    Get-ChildItem -Force |
        Where-Object { $_.Name -ne "include" -and $_.Name -ne "single_include" } |
        Remove-Item -Recurse -Force

    Set-Location -Path $PSScriptRoot
}
```

3. **Build using CMake:**
```powershell
cmake -B build
cmake --build build --config Release
```

4. **(Optional) Run tests:**
```powershell
cd build
ctest -C Release
```

5. **(Optional) Install:**
```powershell
cmake --install build --config Release
```

---

### Build Options

You can customize the build using the following CMake options:

| Option                               | Description                                                                  | Default |
| ------------------------------------ | ---------------------------------------------------------------------------  | ------- |
| `CLUEAPI_USE_NLOHMANN_JSON`          | Enable **nlohmann/json** support                                             | `ON`    |
| `CLUEAPI_USE_CUSTOM_JSON`            | Enable **custom JSON** support (overrides `nlohmann/json`)                   | `OFF`   |
| `CLUEAPI_USE_LOGGING_MODULE`         | Enable the **logging module**                                                | `ON`    |
| `CLUEAPI_USE_DOTENV_MODULE`          | Enable the **dotenv module**                                                 | `ON`    |
| `CLUEAPI_USE_REDIS_MODULE`           | Enable the **Redis module**                                                  | `ON`    |
| `CLUEAPI_USE_RTTI`                   | Enable **Run-Time Type Information (RTTI)**                                  | `OFF`   |
| `CLUEAPI_ENABLE_ASAN`                | Enable **AddressSanitizer (ASan)**                                           | `OFF`   |
| `CLUEAPI_ENABLE_TSAN`                | Enable **ThreadSanitizer (TSan)**                                            | `OFF`   |
| `CLUEAPI_ENABLE_UBSAN`               | Enable **UndefinedBehaviorSanitizer (UBSan)**                                | `OFF`   |
| `CLUEAPI_ENABLE_IPO`                 | Enable **Interprocedural Optimization (IPO/LTO)**                            | `ON`    |
| `CLUEAPI_ENABLE_EXTRA_OPTIMIZATIONS` | Enable **extra compiler optimizations**                                      | `ON`    |
| `CLUEAPI_ENABLE_WARNINGS`            | Enable **extra compiler warnings**                                           | `OFF`   |
| `CLUEAPI_BUILD_TESTS`                | Build and enable tests                                                       | `OFF`   |
| `CLUEAPI_OPTIMIZED_LOG_LEVEL`        | Optimized log level: `TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL, NONE`    | `INFO`  |

## Roadmap

Future development will focus on expanding the ecosystem and adding more enterprise-grade features:

* **OpenAPI Integration**: Automatic generation of OpenAPI (Swagger) specifications from route definitions for seamless API documentation and client generation.
* **Database Modules**: Dedicated modules for interacting with popular databases like **PostgreSQL**, providing connection pooling and asynchronous query execution.
* **Metrics & Observability**: A module to expose application metrics in the **Prometheus** format for easy integration into modern monitoring and alerting pipelines.
* **WebSocket Support**: First-class support for WebSocket connections to enable real-time, bidirectional communication.

## Contributing

Contributions are highly welcome. Please feel free to open an issue to discuss your ideas or submit a pull request with your improvements.

## License

This project is licensed under the **MIT License**.