cmake_minimum_required(VERSION 3.20)

project(clueapi VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake ${CMAKE_MODULE_PATH})

include(cmake/policies.cmake)

include(cmake/options.cmake)

include(cmake/sources.cmake)

include(cmake/packages.cmake)

include(cmake/target_utils.cmake)

include(cmake/target.cmake)

include(cmake/install.cmake)

include(cmake/packaging.cmake)

include(cmake/tests.cmake)