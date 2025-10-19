add_library(clueapi ${CLUEAPI_SOURCES})
add_library(clueapi::clueapi ALIAS clueapi)

target_compile_features(clueapi PUBLIC cxx_std_20)

target_compile_definitions(clueapi PUBLIC "CLUEAPI_OPTIMIZED_LOG_LEVEL=${CLUEAPI_LOG_LEVEL_NUM}")

if(CLUEAPI_USE_NLOHMANN_JSON)
    target_include_directories(clueapi PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/clueapi/shared/thirdparty/nlohmann_json/single_include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/clueapi/thirdparty/nlohmann_json>
    )

    target_compile_definitions(clueapi PUBLIC CLUEAPI_USE_NLOHMANN_JSON)
endif()

if(NOT TARGET fmt::fmt)
    target_compile_definitions(clueapi PUBLIC FMT_HEADER_ONLY=1)
    
    target_include_directories(clueapi PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/clueapi/shared/thirdparty/fmt/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/clueapi/thirdparty/fmt>
    )
else()
    target_link_libraries(clueapi PUBLIC fmt::fmt)
endif()

target_include_directories(clueapi PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/clueapi/shared/thirdparty/ankerl_unordered_dense/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/clueapi/thirdparty/ankerl_unordered_dense>
)

target_include_directories(clueapi PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/clueapi/shared/thirdparty/tl-expected/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/clueapi/thirdparty/tl-expected>
)

target_include_directories(clueapi
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/clueapi>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/clueapi>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/clueapi/shared
)

target_link_libraries(clueapi PUBLIC Boost::system Boost::filesystem Boost::iostreams)
target_link_libraries(clueapi PUBLIC Threads::Threads)
target_link_libraries(clueapi PUBLIC OpenSSL::SSL OpenSSL::Crypto)

if(CLUEAPI_USE_LOGGING_MODULE)
    target_compile_definitions(clueapi PUBLIC CLUEAPI_USE_LOGGING_MODULE)
endif()

if(CLUEAPI_USE_DOTENV_MODULE)
    target_compile_definitions(clueapi PUBLIC CLUEAPI_USE_DOTENV_MODULE)
endif()

if(CLUEAPI_USE_REDIS_MODULE)
    target_compile_definitions(clueapi PUBLIC CLUEAPI_USE_REDIS_MODULE)
endif()

configure_target(clueapi ENABLE_IO_URING ON)