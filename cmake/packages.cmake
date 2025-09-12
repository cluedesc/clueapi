find_package(Boost 1.84.0 REQUIRED COMPONENTS system filesystem iostreams)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found")
endif()

find_package(OpenSSL REQUIRED)

if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found")
endif()

find_package(Threads REQUIRED)

if(NOT Threads_FOUND)
    message(FATAL_ERROR "Threads not found")
endif()