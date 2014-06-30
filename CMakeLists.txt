cmake_minimum_required(VERSION 2.8.9 FATAL_ERROR)
project(LIBRORC)

set( REQUIRED_PDA_VERSION "8:0:2" )
set(VERSION "6.0.0")

include_directories ( ${LIBRORC_SOURCE_DIR}/include )
set( CMAKE_CXX_FLAGS_DEBUG  "-Wall -O0 -g" )
set( CMAKE_CXX_FLAGS "-Wall -g" )

MESSAGE( STATUS "-------------------------------------------------------------------------------")

#--- Find and check PDA -------------------------------------------------------

include(CMakePda.txt)

#--- Compile LibRORC in Simulation mode ---------------------------------------

option(MyOption "MODELSIM" OFF)
if(${MODELSIM})
    add_definitions(-DMODELSIM)
    MESSAGE( STATUS "Librorc is going to be compiled in simulation mode!")
endif(${MODELSIM})

#--- Add target for Doxygen ---------------------------------------------------

find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)

add_subdirectory(src)
add_subdirectory(fli)

#--- build libraries and tools ------------------------------------------------

OPTION(MyOption "LIBMODE" SHARED)

# Build librorc
add_library( rorc ${LIBMODE} $<TARGET_OBJECTS:LIBRORC_OBJECTS> )

SET_TARGET_PROPERTIES( rorc PROPERTIES VERSION ${VERSION} SOVERSION 0)

target_link_libraries( rorc pthread pci ${EXTRA_LIBS})

# Build flisock adapter (if MODELSIM is configured)
IF(${MODELSIM})
    MESSAGE( STATUS "Adding (optional) FLI Socket Interface")
    add_library( fli ${LIBMODE} $<TARGET_OBJECTS:FLI_OBJECTS> )
    set_target_properties(fli PROPERTIES PREFIX "")
ENDIF()

# Build executables (tests and tools)
add_subdirectory(tools_src)

#--- install -----------------------------------------------------------------

FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h*")
INSTALL(FILES ${files} DESTINATION include)

FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/include/librorc/*.h*")
INSTALL(FILES ${files} DESTINATION include/librorc)

INSTALL(TARGETS rorc LIBRARY DESTINATION lib )

#--- Setup CPack --------------------------------------------------------------

include(CPack.txt)

MESSAGE( STATUS "-------------------------------------------------------------------------------")
