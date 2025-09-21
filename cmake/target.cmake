add_library(clueapi ${CLUEAPI_SOURCES})
add_library(clueapi::clueapi ALIAS clueapi)

configure_target(clueapi 
    USE_RTTI ${CLUEAPI_USE_RTTI}
    ENABLE_IPO ${CLUEAPI_ENABLE_IPO}
    ENABLE_ASAN ${CLUEAPI_ENABLE_ASAN}
    ENABLE_UBSAN ${CLUEAPI_ENABLE_UBSAN}
    ENABLE_TSAN ${CLUEAPI_ENABLE_TSAN}
    ENABLE_WARNINGS ${CLUEAPI_ENABLE_WARNINGS}
    ENABLE_OPTIMIZATIONS ${CLUEAPI_ENABLE_EXTRA_OPTIMIZATIONS}
    ENABLE_IO_URING ON
    ENABLE_CUSTOM_LINKER ON
)

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

target_include_directories(clueapi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clueapi>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared
)

target_link_libraries(clueapi PUBLIC Boost::system Boost::filesystem Boost::iostreams)

target_link_libraries(clueapi PUBLIC OpenSSL::SSL OpenSSL::Crypto)

target_link_libraries(clueapi PUBLIC Threads::Threads)

target_precompile_headers(clueapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/clueapi/shared/pch/pch.hxx)