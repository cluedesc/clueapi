cmake_minimum_required(VERSION 3.21)

project(clueapi VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake ${CMAKE_MODULE_PATH})

include(cmake/policies.cmake)

include(cmake/options.cmake)

include(cmake/packages.cmake)

include(cmake/sources.cmake)

include(cmake/target_utils.cmake)

include(cmake/target.cmake)

if(CLUEAPI_IS_TOP_LEVEL)
    include(cmake/install.cmake)

    include(cmake/packaging.cmake)
endif()

include(cmake/tests.cmake)