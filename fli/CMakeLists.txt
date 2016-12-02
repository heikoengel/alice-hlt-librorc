project(flisock)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
add_library(fli SHARED flisock.c)
INSTALL(TARGETS fli LIBRARY DESTINATION lib)
