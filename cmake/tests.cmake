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

if(CLUEAPI_USE_REDIS_MODULE)
    list(APPEND CLUEAPI_TEST_SOURCES tests/modules/redis/redis.cxx)
    list(APPEND CLUEAPI_TEST_SOURCES tests/modules/redis/connection/connection.cxx)
endif()

add_executable(clueapi_tests ${CLUEAPI_TEST_SOURCES})

configure_target(clueapi_tests 
    USE_RTTI ${CLUEAPI_USE_RTTI}
    ENABLE_IPO OFF
    ENABLE_ASAN ${CLUEAPI_ENABLE_ASAN}
    ENABLE_UBSAN ${CLUEAPI_ENABLE_UBSAN}
    ENABLE_TSAN ${CLUEAPI_ENABLE_TSAN}
    ENABLE_WARNINGS ${CLUEAPI_ENABLE_WARNINGS}
    ENABLE_OPTIMIZATIONS ${CLUEAPI_ENABLE_EXTRA_OPTIMIZATIONS}
    ENABLE_IO_URING ON
    ENABLE_CUSTOM_LINKER ON
)

target_link_libraries(clueapi_tests
    PRIVATE
    clueapi
    GTest::gtest_main
)

if (CLUEAPI_ENABLE_IPO)
    message(STATUS "Forcing IPO for clueapi_tests")

    set_target_properties(clueapi_tests PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
endif()

include(GoogleTest)

gtest_discover_tests(clueapi_tests)