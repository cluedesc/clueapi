if(NOT CLUEAPI_IS_TOP_LEVEL)
    return()
endif()

set(CLUEAPI_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/clueapi)

install(TARGETS clueapi
    EXPORT clueapi-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/clueapi/
    DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hxx"
    PATTERN "*.hpp"
    PATTERN "modules/logging/*" EXCLUDE
    PATTERN "modules/dotenv/*" EXCLUDE
    PATTERN "modules/redis/*" EXCLUDE
    PATTERN "shared/thirdparty/nlohmann_json/*" EXCLUDE
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/clueapi/shared/thirdparty/ankerl_unordered_dense/
    DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/thirdparty/ankerl_unordered_dense
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hxx" PATTERN "*.hpp"
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/clueapi/shared/thirdparty/tl-expected/
    DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/thirdparty/tl-expected
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hxx" PATTERN "*.hpp"
)

if(CLUEAPI_USE_NLOHMANN_JSON)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/clueapi/shared/thirdparty/nlohmann_json/
        DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/thirdparty/nlohmann_json
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hxx" PATTERN "*.hpp"
    )
endif()

if(CLUEAPI_USE_LOGGING_MODULE)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/clueapi/modules/logging/
        DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/modules/logging
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hxx" PATTERN "*.hpp"
    )
endif()

if(CLUEAPI_USE_DOTENV_MODULE)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/clueapi/modules/dotenv/
        DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/modules/dotenv
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hxx" PATTERN "*.hpp"
    )
endif()

if(CLUEAPI_USE_REDIS_MODULE)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/clueapi/modules/redis/
        DESTINATION ${CLUEAPI_INSTALL_INCLUDEDIR}/modules/redis
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hxx" PATTERN "*.hpp"
    )
endif()

install(EXPORT clueapi-targets
    FILE clueapi-targets.cmake
    NAMESPACE clueapi::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clueapi
)