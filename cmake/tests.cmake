if(NOT CLUEAPI_BUILD_TESTS)
    return()
endif()

enable_testing()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(WIN32)
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

if(CLUEAPI_USE_REDIS_MODULE)
    list(APPEND CLUEAPI_TEST_SOURCES tests/modules/redis/redis.cxx)
    list(APPEND CLUEAPI_TEST_SOURCES tests/modules/redis/connection/connection.cxx)
endif()

add_executable(clueapi_tests ${CLUEAPI_TEST_SOURCES})

configure_target(clueapi_tests 
    ENABLE_IO_URING ON
)

target_link_libraries(clueapi_tests
    PRIVATE
    clueapi
    GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(clueapi_tests)