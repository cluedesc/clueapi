function(configure_target_encoding TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    )
endfunction()

function(configure_target_io_uring TARGET_NAME)
    if(UNIX AND NOT APPLE)
        find_package(PkgConfig QUIET)
        
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(URING liburing QUIET)
            
            if(URING_FOUND)
                if(CLUEAPI_IS_TOP_LEVEL)
                    message(STATUS "io_uring support enabled for target: ${TARGET_NAME}")
                endif()

                target_include_directories(${TARGET_NAME} PRIVATE ${URING_INCLUDE_DIRS})
                target_link_libraries(${TARGET_NAME} PRIVATE ${URING_LIBRARIES})

                target_compile_definitions(${TARGET_NAME} PUBLIC BOOST_ASIO_HAS_IO_URING=1)
            else()
                if(CLUEAPI_IS_TOP_LEVEL)
                    message(STATUS "Liburing not found via pkg-config, trying direct link for target: ${TARGET_NAME}")
                endif()

                find_library(URING_LIB uring)

                if(URING_LIB)
                    target_link_libraries(${TARGET_NAME} PRIVATE ${URING_LIB})

                    target_compile_definitions(${TARGET_NAME} PUBLIC BOOST_ASIO_HAS_IO_URING=1)
                    
                    if(CLUEAPI_IS_TOP_LEVEL)
                        message(STATUS "io_uring support enabled via direct link for target: ${TARGET_NAME}")
                    endif()
                else()
                    if(CLUEAPI_IS_TOP_LEVEL)
                        message(STATUS "io_uring not available for target: ${TARGET_NAME}")
                    endif()
                endif()
            endif()
        else()
            if(CLUEAPI_IS_TOP_LEVEL)
                message(STATUS "PkgConfig not found, io_uring support disabled for target: ${TARGET_NAME}")
            endif()
        endif()
    else()
        if(CLUEAPI_IS_TOP_LEVEL)
            message(STATUS "io_uring not available on this platform for target: ${TARGET_NAME}")
        endif()
    endif()
endfunction()

function(configure_target TARGET_NAME)
    cmake_parse_arguments(
        ARG
        ""
        "ENABLE_IO_URING"
        ""
        ${ARGN}
    )
    
    if(CLUEAPI_IS_TOP_LEVEL)
        message(STATUS "Configuring target: ${TARGET_NAME}")
    endif()

    if(MSVC)
        set_property(TARGET ${TARGET_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    configure_target_encoding(${TARGET_NAME})
    
    if(${ARG_ENABLE_IO_URING})
        configure_target_io_uring(${TARGET_NAME})
    endif()
endfunction()