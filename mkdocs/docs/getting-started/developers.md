# Getting Started for Developers

Welcome to `clueapi`! This guide will walk you through the process of integrating the framework into your own C++ projects. We will cover system-wide installation and modern integration using CMake's `FetchContent`.

---

## Prerequisites

Before you start, ensure your development environment meets the following requirements:

* **C++20 Compiler**: You need a compiler with full C++20 support (e.g., GCC 10+, Clang 12+, MSVC 2019 v16.10+).
* **CMake**: Your project should use CMake version 3.20 or higher.
* **Boost**: You must have Boost version 1.84.0 or higher installed, with the `system`, `filesystem`, and `iostreams` libraries available.
* **OpenSSL**: The OpenSSL library is required for cryptographic functions used by the underlying libraries.
* **liburing**: For optimal performance on Linux, installing `liburing` enables the `io_uring` asynchronous I/O interface.

---

## Installation

You have two primary methods for integrating `clueapi` into your project: installing it system-wide or fetching it directly within your CMake script.

### Option 1: System-Wide Installation

This method involves building and installing `clueapi` on your system, making it available for any project via CMake's `find_package`.

1.  **Clone and Build `clueapi`**:
First, clone the `clueapi` repository and its submodules. Then, build and install it.

```bash
# Clone the repository
git clone --recurse-submodules [https://github.com/cluedesc/clueapi.git](https://github.com/cluedesc/clueapi.git)

cd clueapi

# Configure the build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build build -j $(nproc)

# Install the library and headers system-wide
sudo cmake --install build
```

2.  **Find `clueapi` in Your Project**:
In your project's `CMakeLists.txt`, you can now find and link against the installed library.

```cmake
find_package(clueapi REQUIRED)

# ... later in your CMakeLists.txt
target_link_libraries(your_executable PRIVATE clueapi::clueapi)
```

### Option 2: Installation with CMake FetchContent (Recommended)

This is the recommended approach for modern CMake projects as it doesn't require any pre-installation steps and makes your project self-contained.`FetchContent` will download and configure `clueapi` as part of your project's build process.

Simply add the following to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    clueapi
    GIT_REPOSITORY [https://github.com/cluedesc/clueapi.git](https://github.com/cluedesc/clueapi.git)
    GIT_TAG main # Or a specific release tag, e.g., v1.0.0
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES ""   # Let clueapi's build handle its own submodules
)

FetchContent_MakeAvailable(clueapi)

# ... later in your CMakeLists.txt
target_link_libraries(your_executable PRIVATE clueapi::clueapi)
```

---

## Configuration

Here is a minimal `CMakeLists.txt` for an executable that uses `clueapi`. This example assumes you are using the `FetchContent` method.

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_api_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add your executable
add_executable(my_api_server main.cxx)

# --- Fetch and link clueapi ---
include(FetchContent)

FetchContent_Declare(
    clueapi
    GIT_REPOSITORY [https://github.com/cluedesc/clueapi.git](https://github.com/cluedesc/clueapi.git)
    GIT_TAG main
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES ""
)

FetchContent_MakeAvailable(clueapi)

# Find required packages that clueapi depends on
find_package(Boost 1.84.0 REQUIRED COMPONENTS system filesystem iostreams)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Link clueapi library to your executable
target_link_libraries(my_api_server
    PRIVATE
    clueapi::clueapi
)
```

You can set options for the build using `-D<option>=<value>` flags.

| Option                        | Description                                                             | Default |
| ----------------------------- | ----------------------------------------------------------------------- | ------- |
| `CLUEAPI_USE_NLOHMANN_JSON`   | Enable nlohmann/json support                                            | `ON`    |
| `CLUEAPI_USE_LOGGING_MODULE`  | Enable the logging module                                               | `ON`    |
| `CLUEAPI_USE_DOTENV_MODULE`   | Enable the dotenv module                                                | `ON`    |
| `CLUEAPI_USE_RTTI`            | Enable Run-Time Type Information                                        | `OFF`   |
| `CLUEAPI_OPTIMIZED_LOG_LEVEL` | Optimized log level: TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL, NONE | `INFO`  |
| `CLUEAPI_RUN_TESTS`           | Build and enable tests                                                  | `OFF`   |

## Basic Usage

Here is a simple example of a "Hello, World!" server. Save this code as `main.cxx` in your project directory.

```cpp
#include <clueapi.hxx>
#include <iostream>

// A simple synchronous handler for the root path.
clueapi::http::types::response_t get_root(clueapi::http::ctx_t ctx) {
    // Create a plain text response.
    clueapi::http::types::text_response_t response;

    {
        response.body() = "Hello, world!";
        response.status() = clueapi::http::types::status_t::ok;
    }

    return response;
}

int main() {
    // Create an instance of the API.
    auto api = clueapi::api();

    // Create a default configuration.
    auto cfg = clueapi::cfg_t{};

    {
        cfg.m_host = "127.0.0.1";
        cfg.m_port = "8080";

        cfg.m_server.m_workers = std::thread::hardware_concurrency(); // Use all available cores.
    }

    // Add a GET route for the path "/".
    api.add_method(
        clueapi::http::types::method_t::get,

        "/",

        get_root
    );

    std::cout << "Server starting at http://" << cfg.m_host << ":" << cfg.m_port << std::endl;

    // Start the server with the configuration.
    api.start(cfg);

    // Block the main thread to keep the server running.
    // The server will handle shutdown signals (like Ctrl+C) gracefully.
    api.wait();

    // The stop call is optional as wait() blocks until shutdown,
    // but it's good practice for explicit cleanup.
    api.stop();

    std::cout << "Server stopped." << std::endl;

    return 0;
}
```

After setting up your `CMakeLists.txt` and `main.cxx`, you can build and run your server:

```bash
# Configure the project
cmake -B build

# Build the executable
cmake --build build

# Run your server
./build/my_api_server
```

You can now access your server by navigating to `http://127.0.0.1:8080` in your web browser or using a tool like `curl`.

More examples can be found in the 'Examples' section! :)