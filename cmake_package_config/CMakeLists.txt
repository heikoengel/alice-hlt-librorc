CONFIGURE_FILE(librorc-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/librorc-config-version.cmake
  @ONLY)

INSTALL( FILES librorc-config.cmake DESTINATION lib/cmake/librorc )
INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/librorc-config-version.cmake DESTINATION lib/cmake/librorc )
