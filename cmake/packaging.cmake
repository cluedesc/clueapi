configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/clueapi-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clueapi
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/clueapi-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clueapi
)