# Getting Started for Contributors

Thank you for your interest in contributing to `clueapi`! This guide provides all the necessary information to get you set up with a local development environment, from cloning the repository to running the test suite.

---

## Prerequisites

Before you begin, ensure you have the following dependencies installed on your system. The project relies on a modern C++ toolchain and several industry-standard libraries.

* **C++20 Compiler**: A compiler with full support for C++20 is required (e.g., GCC 10+, Clang 12+, MSVC 2019 v16.10+).
* **CMake**: The build system requires CMake version 3.20 or higher.
* **Boost**: You need Boost version 1.84.0 or higher, with the `system`, `filesystem`, and `iostreams` components compiled.
* **OpenSSL**: The library is required for cryptographic functions.
* **liburing**: For optimal performance on Linux, installing `liburing` enables the `io_uring` asynchronous I/O interface.
* **Git**: Required for cloning the repository and managing submodules.

---

## Installation

To get started, clone the repository and initialize its submodules. The project uses several third-party libraries which are included as Git submodules.

1. **Clone the repository with submodules:**
```bash
git clone --recurse-submodules https://github.com/cluedesc/clueapi.git

cd clueapi
```

2. **Remove unnecessary files from submodules (Op):**

    **Linux (Bash):**
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

    **Windows (PowerShell):**
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

    Get-ChildItem -File -Force |
        Where-Object { $_.Name -ne "include" -and $_.Name -ne "single_include" } |
        Remove-Item -Recurse -Force

    Get-ChildItem -Directory -Force |
        Where-Object { $_.Name -ne "include" -and $_.Name -ne "single_include" } |
        Remove-Item -Recurse -Force

    Set-Location -Path $PSScriptRoot
}
```

---

## Configuration

The project uses CMake for building. You can configure the build by passing options to the CMake command. These flags allow you to tailor the build to your specific needs, such as enabling or disabling modules and features.

* **Create a build directory:** It's best practice to create an out-of-source build directory.
```bash
cmake -B build
```

* **Customize the build (Optional):** You can pass `-D<option>=<value>` flags to CMake. For example, to disable RTTI:
```bash
cmake -B build -DCLUEAPI_RUN_TESTS=OFF -DCLUEAPI_USE_RTTI=OFF
```

Here are the primary build options available:

| Option                        | Description                                                             | Default |
| ----------------------------- | ----------------------------------------------------------------------- | ------- |
| `CLUEAPI_USE_NLOHMANN_JSON`   | Enable nlohmann/json support                                            | `ON`    |
| `CLUEAPI_USE_LOGGING_MODULE`  | Enable the logging module                                               | `ON`    |
| `CLUEAPI_USE_DOTENV_MODULE`   | Enable the dotenv module                                                | `ON`    |
| `CLUEAPI_USE_RTTI`            | Enable Run-Time Type Information                                        | `OFF`   |
| `CLUEAPI_OPTIMIZED_LOG_LEVEL` | Optimized log level: TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL, NONE | `INFO`  |
| `CLUEAPI_RUN_TESTS`           | Build and enable tests                                                  | `OFF`   |

---

## Building

Once the project is configured, you can build it using the CMake `--build` command.

* **Build the project:** This command will compile the library and all enabled targets (like the test executable). The `-j` flag specifies the number of parallel jobs to use.
```bash
cmake --build build -j $(nproc)
```

The compiled library (`libclueapi.a` or `clueapi.so`) and the test runner (`clueapi_tests`) will be located in the `build` directory.

---

## Testing

The project has a comprehensive test suite built with GoogleTest to ensure code quality and correctness. The test suite is automatically configured if `CLUEAPI_RUN_TESTS` is `ON`.

* **Run the tests:** After building the project, you can run all tests using `ctest` from within the build directory.
```bash
cd build

ctest --output-on-failure
```

All tests should pass. If you encounter any failures, please ensure your environment is set up correctly before submitting a pull request.

---

## Contributing

We welcome contributions of all kinds, from bug fixes to new features and documentation improvements.

* **Code Style**: The project follows a specific code style defined in the `.clang-format` file. Please format your code using `clang-format` before committing to ensure consistency.
* **Branches**: Create a new feature branch for your changes (e.g., `feature/add-new-middleware` or `fix/routing-bug`).
* **Pull Requests**: Open a pull request against the `main` branch. Provide a clear description of the changes you have made and reference any relevant issues.
* **Issues**: Feel free to open an issue to discuss your ideas, report a bug, or suggest an improvement.

Thank you for helping make `clueapi` better!