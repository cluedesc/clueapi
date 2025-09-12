function(configure_target_ipo TARGET_NAME)
    include(CheckIPOSupported)

    check_ipo_supported(RESULT ipo_supported OUTPUT error)
    
    if(ipo_supported)
        message(STATUS "IPO/LTO enabled for target: ${TARGET_NAME}")

        set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        
        if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        endif()
    else()
        message(STATUS "IPO/LTO not supported for target ${TARGET_NAME}: ${error}")
    endif()
endfunction()

function(configure_target_rtti TARGET_NAME USE_RTTI)
    if(${USE_RTTI})
        message(STATUS "RTTI enabled for target: ${TARGET_NAME}")
    else()
        message(STATUS "RTTI disabled for target: ${TARGET_NAME}")
        
        target_compile_options(${TARGET_NAME} PRIVATE 
            $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-rtti>
            $<$<CXX_COMPILER_ID:MSVC>:/GR->
        )
    endif()
endfunction()

function(configure_target_encoding TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    )
endfunction()

function(configure_target_linker TARGET_NAME)
    if(NOT WIN32)
        find_program(MOLD_PATH mold)
        find_program(LLD_PATH ld.lld)
        
        if(MOLD_PATH)
            message(STATUS "Using Mold linker for target: ${TARGET_NAME}")

            target_link_options(${TARGET_NAME} PRIVATE -fuse-ld=mold)
        elseif(LLD_PATH)
            message(STATUS "Using LLD linker for target: ${TARGET_NAME}")

            target_link_options(${TARGET_NAME} PRIVATE -fuse-ld=lld)
        else()
            message(STATUS "Using default linker for target: ${TARGET_NAME}")
        endif()
        
        target_link_options(${TARGET_NAME} PRIVATE -Wl,--as-needed)
    endif()
endfunction()

function(configure_target_optimizations TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            $<$<CXX_COMPILER_ID:GNU,Clang>:-O0 -g3 -fno-omit-frame-pointer>
            $<$<CXX_COMPILER_ID:MSVC>:/Od /Zi>
        >
        $<$<CONFIG:Release>:
            $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -DNDEBUG -fomit-frame-pointer>
            $<$<CXX_COMPILER_ID:MSVC>:/O2 /DNDEBUG>
        >
        $<$<CONFIG:RelWithDebInfo>:
            $<$<CXX_COMPILER_ID:GNU,Clang>:-O2 -g1 -DNDEBUG>
            $<$<CXX_COMPILER_ID:MSVC>:/O2 /Zi /DNDEBUG>
        >
    )
endfunction()

function(configure_target_warnings TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -Wall -Wextra -Wpedantic
            -Wcast-align -Wcast-qual -Wshadow -Wconversion
            -Wformat=2 -Wundef
            -Wswitch-default -Wmissing-declarations
            -Wmissing-include-dirs -Wredundant-decls
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W3 /permissive-
        >
    )
endfunction()

function(configure_target_sanitizers TARGET_NAME ENABLE_ASAN ENABLE_UBSAN ENABLE_TSAN)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SANITIZER_FLAGS "")
        
        if(${ENABLE_ASAN})
            list(APPEND SANITIZER_FLAGS "-fsanitize=address" "-fno-omit-frame-pointer")

            message(STATUS "AddressSanitizer enabled for target: ${TARGET_NAME}")
        endif()
        
        if(${ENABLE_UBSAN})
            list(APPEND SANITIZER_FLAGS "-fsanitize=undefined")

            message(STATUS "UndefinedBehaviorSanitizer enabled for target: ${TARGET_NAME}")
        endif()
        
        if(${ENABLE_TSAN})
            if(${ENABLE_ASAN})
                message(WARNING "ThreadSanitizer cannot be used with AddressSanitizer")
            else()
                list(APPEND SANITIZER_FLAGS "-fsanitize=thread")

                message(STATUS "ThreadSanitizer enabled for target: ${TARGET_NAME}")
            endif()
        endif()
        
        if(SANITIZER_FLAGS)
            target_compile_options(${TARGET_NAME} PRIVATE ${SANITIZER_FLAGS})
            target_link_options(${TARGET_NAME} PRIVATE ${SANITIZER_FLAGS})
        endif()
    endif()
endfunction()

function(configure_target_io_uring TARGET_NAME)
    if(UNIX AND NOT APPLE)
        find_package(PkgConfig)
        
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(URING liburing)
            
            if(URING_FOUND)
                message(STATUS "io_uring support enabled for target: ${TARGET_NAME}")

                target_include_directories(${TARGET_NAME} PRIVATE ${URING_INCLUDE_DIRS})
                target_link_libraries(${TARGET_NAME} PRIVATE ${URING_LIBRARIES})
                target_compile_definitions(${TARGET_NAME} PRIVATE BOOST_ASIO_HAS_IO_URING=1)
            else()
                message(STATUS "Liburing not found via pkg-config, trying direct link for target: ${TARGET_NAME}")

                find_library(URING_LIB uring)

                if(URING_LIB)
                    target_link_libraries(${TARGET_NAME} PRIVATE ${URING_LIB})
                    target_compile_definitions(${TARGET_NAME} PRIVATE BOOST_ASIO_HAS_IO_URING=1)

                    message(STATUS "io_uring support enabled via direct link for target: ${TARGET_NAME}")
                else()
                    message(STATUS "io_uring not available for target: ${TARGET_NAME}")
                endif()
            endif()
        else()
            message(STATUS "PkgConfig not found, io_uring support disabled for target: ${TARGET_NAME}")
        endif()
    else()
        message(STATUS "io_uring not available on this platform for target: ${TARGET_NAME}")
    endif()
endfunction()

function(configure_target TARGET_NAME)
    cmake_parse_arguments(
        ARG
        ""
        "USE_RTTI;ENABLE_IPO;ENABLE_ASAN;ENABLE_UBSAN;ENABLE_TSAN;ENABLE_WARNINGS;ENABLE_OPTIMIZATIONS;ENABLE_IO_URING;ENABLE_CUSTOM_LINKER"
        ""
        ${ARGN}
    )
    
    message(STATUS "Configuring target: ${TARGET_NAME}")

    if(MSVC)
        set_property(TARGET ${TARGET_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/std:c++20>
    )
    
    if (${ARG_ENABLE_CUSTOM_LINKER})
        configure_target_linker(${TARGET_NAME})
    endif()

    configure_target_encoding(${TARGET_NAME})
    
    configure_target_rtti(${TARGET_NAME} ${ARG_USE_RTTI})
    
    if(${ARG_ENABLE_IPO})
        configure_target_ipo(${TARGET_NAME})
    endif()
    
    if(${ARG_ENABLE_OPTIMIZATIONS})
        configure_target_optimizations(${TARGET_NAME})
    endif()
    
    if(${ARG_ENABLE_WARNINGS})
        configure_target_warnings(${TARGET_NAME})
    endif()
    
    if(${ARG_ENABLE_ASAN} OR ${ARG_ENABLE_UBSAN} OR ${ARG_ENABLE_TSAN})
        configure_target_sanitizers(${TARGET_NAME} ${ARG_ENABLE_ASAN} ${ARG_ENABLE_UBSAN} ${ARG_ENABLE_TSAN})
    endif()
    
    if(${ARG_ENABLE_IO_URING})
        configure_target_io_uring(${TARGET_NAME})
    endif()
endfunction()