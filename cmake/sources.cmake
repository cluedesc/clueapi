file(GLOB_RECURSE CLUEAPI_SOURCES ${PROJECT_SOURCE_DIR}/clueapi/*.cxx)

if(NOT CLUEAPI_SOURCES)
    message(FATAL_ERROR "No source files found in ${PROJECT_SOURCE_DIR}/clueapi/")
endif()

if(NOT CLUEAPI_USE_LOGGING_MODULE)
    list(FILTER CLUEAPI_SOURCES EXCLUDE REGEX "/modules/logging/")
endif()

if(NOT CLUEAPI_USE_DOTENV_MODULE)
    list(FILTER CLUEAPI_SOURCES EXCLUDE REGEX "/modules/dotenv/")
endif()

if(NOT CLUEAPI_USE_REDIS_MODULE)
    list(FILTER CLUEAPI_SOURCES EXCLUDE REGEX "/modules/redis/")
endif()