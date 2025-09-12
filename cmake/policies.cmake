foreach(POL CMP0144 CMP0167)
    if(POLICY ${POL})
        cmake_policy(SET ${POL} NEW)
    endif()
endforeach()

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)