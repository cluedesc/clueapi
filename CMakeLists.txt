cmake_minimum_required(VERSION 3.20)

project(clueapi VERSION 1.0.0 LANGUAGES CXX)

foreach(POL CMP0144 CMP0167 CMP0135)
    if(POLICY ${POL})
        cmake_policy(SET ${POL} NEW)
    endif()
endforeach()

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(Boost_USE_STATIC_LIBS ON)

option(CLUEAPI_USE_NLOHMANN_JSON "Enable nlohmann/json support" ON)
option(CLUEAPI_USE_CUSTOM_JSON "Enable custom json support" OFF)

option(CLUEAPI_USE_LOGGING_MODULE "Enable logging module support" ON)
option(CLUEAPI_USE_DOTENV_MODULE "Enable dotenv module support" ON)

option(CLUEAPI_USE_RTTI "Use RTTI" OFF)

option(CLUEAPI_RUN_TESTS "Run tests" OFF)

set(CLUEAPI_OPTIMIZED_LOG_LEVEL INFO CACHE STRING "Optimized log level: TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL, NONE")

set_property(CACHE CLUEAPI_OPTIMIZED_LOG_LEVEL PROPERTY STRINGS TRACE DEBUG INFO WARNING ERROR CRITICAL NONE)

set(LOG_LEVELS "TRACE;DEBUG;INFO;WARNING;ERROR;CRITICAL;NONE")

list(FIND LOG_LEVELS ${CLUEAPI_OPTIMIZED_LOG_LEVEL} CLUEAPI_LOG_LEVEL_NUM)

if(CLUEAPI_LOG_LEVEL_NUM EQUAL -1)
    set(CLUEAPI_LOG_LEVEL_NUM 2)

    message(WARNING "Unknown log level: ${CLUEAPI_OPTIMIZED_LOG_LEVEL}, defaulting to INFO (2)")
endif()

add_compile_definitions(CLUEAPI_OPTIMIZED_LOG_LEVEL=${CLUEAPI_LOG_LEVEL_NUM})

if(CLUEAPI_USE_CUSTOM_JSON)
    set(CLUEAPI_USE_NLOHMANN_JSON OFF)
endif()

message(STATUS "CLUEAPI configuration summary:")
message(STATUS "  CLUEAPI_USE_NLOHMANN_JSON = ${CLUEAPI_USE_NLOHMANN_JSON}")
message(STATUS "  CLUEAPI_USE_CUSTOM_JSON = ${CLUEAPI_USE_CUSTOM_JSON}")
message(STATUS "  CLUEAPI_USE_LOGGING_MODULE = ${CLUEAPI_USE_LOGGING_MODULE}")
message(STATUS "  CLUEAPI_OPTIMIZED_LOG_LEVEL = ${CLUEAPI_OPTIMIZED_LOG_LEVEL} (${CLUEAPI_LOG_LEVEL_NUM})")
message(STATUS "  CLUEAPI_USE_DOTENV_MODULE = ${CLUEAPI_USE_DOTENV_MODULE}")
message(STATUS "  CLUEAPI_USE_RTTI = ${CLUEAPI_USE_RTTI}")
message(STATUS "  CLUEAPI_RUN_TESTS = ${CLUEAPI_RUN_TESTS}")

file(GLOB_RECURSE CLUEAPI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/*.cxx)

if(NOT CLUEAPI_USE_LOGGING_MODULE)
    list(FILTER CLUEAPI_SOURCES EXCLUDE REGEX "/logging/")
endif()

if(NOT CLUEAPI_USE_DOTENV_MODULE)
    list(FILTER CLUEAPI_SOURCES EXCLUDE REGEX "/dotenv/")
endif()

find_program(MOLD_PATH mold)
find_program(LLD_PATH ld.lld)

if(MOLD_PATH)
    message(STATUS "Linker: Mold")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
elseif(LLD_PATH)
    message(STATUS "Linker: lld")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
endif()

if(UNIX)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
endif()

add_library(clueapi ${CLUEAPI_SOURCES})
add_library(clueapi::clueapi ALIAS clueapi)

include(CheckIPOSupported)

check_ipo_supported(RESULT ipo_supported)

if(ipo_supported)
    message(STATUS "IPO/LTO enabled")

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

    set_target_properties(clueapi PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(NOT CLUEAPI_USE_RTTI)
    target_compile_options(clueapi PRIVATE -fno-rtti)
endif()

# ...
if(CLUEAPI_USE_NLOHMANN_JSON)
    target_include_directories(clueapi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/thirdparty/nlohmann_json/single_include>
        $<INSTALL_INTERFACE:include>
    )

    target_compile_definitions(clueapi PUBLIC CLUEAPI_USE_NLOHMANN_JSON)
endif()

target_compile_definitions(clueapi PUBLIC FMT_HEADER_ONLY=1)

target_include_directories(clueapi PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/thirdparty/fmt/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(clueapi PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/thirdparty/ankerl_unordered_dense/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(clueapi PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/thirdparty/tl-expected/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Boost 1.84.0 REQUIRED COMPONENTS system filesystem iostreams)

if(Boost_FOUND)
    target_link_libraries(clueapi PUBLIC Boost::system Boost::filesystem Boost::iostreams)
else()
    message(FATAL_ERROR "Boost not found")
endif()

find_package(OpenSSL REQUIRED)

if(OpenSSL_FOUND)
    target_link_libraries(clueapi PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()

find_package(Threads REQUIRED)

if (Threads_FOUND)
    target_link_libraries(clueapi PUBLIC Threads::Threads)
else()
    message(FATAL_ERROR "Threads not found")
endif()

if (UNIX AND NOT APPLE)
    find_package(PkgConfig)

    if (PKG_CONFIG_FOUND)
        pkg_check_modules(URING liburing)

        if (URING_FOUND)
            message(STATUS "Liburing found")

            target_include_directories(clueapi PUBLIC ${URING_INCLUDE_DIRS})
            target_link_libraries(clueapi PUBLIC ${URING_LIBRARIES})

            target_compile_definitions(clueapi PUBLIC BOOST_ASIO_HAS_IO_URING=1)
        else()
            message(FATAL "Liburing not found, io_uring support disabled")
        endif()
    else()
        message(STATUS "Could not find PkgConfig, trying to link with liburing directly")

        target_link_libraries(clueapi PUBLIC uring)

        target_compile_definitions(clueapi PUBLIC BOOST_ASIO_HAS_IO_URING=1)
    endif()
else()
    message(STATUS "io_uring is not available on this platform, using native async I/O or fallback")
endif()

# ...

target_include_directories(clueapi
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clueapi>
    $<INSTALL_INTERFACE:include>

    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared
)

target_precompile_headers(clueapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/pch/pch.hxx)

set(CLUEAPI_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/clueapi)

install(TARGETS clueapi
    EXPORT clueapi-targets

    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/
    DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}

    FILES_MATCHING 

    PATTERN "*.h" 
    PATTERN "*.hxx" 
    PATTERN "*.hpp"

    PATTERN "modules/logging/*" EXCLUDE
    PATTERN "modules/dotenv/*" EXCLUDE

    PATTERN "shared/thirdparty/nlohmann_json/*" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/thirdparty/ankerl_unordered_dense/
    DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/thirdparty/ankerl_unordered_dense

    FILES_MATCHING

    PATTERN "*.h"
    PATTERN "*.hxx"
    PATTERN "*.hpp"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/thirdparty/tl-expected/
    DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/thirdparty/tl-expected

    FILES_MATCHING

    PATTERN "*.h"
    PATTERN "*.hxx"
    PATTERN "*.hpp"
)

if(CLUEAPI_USE_NLOHMANN_JSON)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/thirdparty/nlohmann_json/
        DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/thirdparty/nlohmann_json

        FILES_MATCHING 

        PATTERN "*.h" 
        PATTERN "*.hxx" 
        PATTERN "*.hpp"
    )
endif()

if (CLUEAPI_USE_LOGGING_MODULE)
    target_compile_definitions(clueapi PUBLIC CLUEAPI_USE_LOGGING_MODULE)

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/modules/logging/
        DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/modules/logging

        FILES_MATCHING

        PATTERN "*.h"
        PATTERN "*.hxx"
        PATTERN "*.hpp"
    )
endif()

if (CLUEAPI_USE_DOTENV_MODULE)
    target_compile_definitions(clueapi PUBLIC CLUEAPI_USE_DOTENV_MODULE)

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/modules/dotenv/
        DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/modules/dotenv

        FILES_MATCHING

        PATTERN "*.h"
        PATTERN "*.hxx"
        PATTERN "*.hpp"
    )
endif()

install(EXPORT clueapi-targets
    FILE clueapi-targets.cmake

    NAMESPACE clueapi::

    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clueapi
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/clueapi-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config.cmake

    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clueapi
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config-version.cmake

    VERSION ${PROJECT_VERSION}

    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config-version.cmake

    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clueapi
)

if(CLUEAPI_RUN_TESTS)
    enable_testing()

    message(STATUS "Configuring tests")

    FetchContent_Declare(
        googletest

        URL https://github.com/google/googletest/archive/refs/heads/main.zip
    )

    if (WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(googletest)

    set(
        CLUEAPI_TEST_SOURCES

        tests/main.cxx

        tests/exceptions/exceptions.cxx

        tests/route/route.cxx

        tests/middleware/middleware.cxx

        tests/http/comparators/comparators.cxx
        tests/http/sv_hash/sv_hash.cxx
        tests/http/mime/mime.cxx
        tests/http/chunks/chunks.cxx
        tests/http/multipart/multipart.cxx
        tests/http/ctx/ctx.cxx

        tests/http/types/file.cxx
        tests/http/types/cookie.cxx
        tests/http/types/method.cxx
        tests/http/types/status.cxx
        tests/http/types/request.cxx
        tests/http/types/response.cxx

        tests/shared/json_traits/json_traits.cxx
        tests/shared/io_ctx_pool/io_ctx_pool.cxx
    )

    if(CLUEAPI_USE_LOGGING_MODULE)
        list(APPEND CLUEAPI_TEST_SOURCES tests/modules/logging/logging.cxx)
        list(APPEND CLUEAPI_TEST_SOURCES tests/modules/logging/file_logger.cxx)
    endif()

    if(CLUEAPI_USE_DOTENV_MODULE)
        list(APPEND CLUEAPI_TEST_SOURCES tests/modules/dotenv/dotenv.cxx)
    endif()

    add_executable(clueapi_tests ${CLUEAPI_TEST_SOURCES})

    target_link_libraries(clueapi_tests
        PRIVATE
        clueapi
        GTest::gtest_main
    )

    include(GoogleTest)

    gtest_discover_tests(clueapi_tests)
endif()