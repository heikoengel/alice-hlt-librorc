cmake_minimum_required(VERSION 2.8.9 FATAL_ERROR)

set( REQUIRED_PDA_VERSION "10:1:6" )
set( VERSION "10.0.0" )

set( CMAKE_CXX_FLAGS "-Wall" )
set( CMAKE_CXX_FLAGS_DEBUG  "-Wall -O0 -g -DDEBUG" )

#--- Find and check PDA
include(CMakePda.txt)

#--- Define whether SHM is used or not
option(MyOption "SHM" OFF)
if(${SHM})
    add_definitions(-DSHM)
    MESSAGE( STATUS "Librorc is going to be compiled with SHM support!")
endif(${SHM})

#--- Compile LibRORC in Simulation mode
option(MyOption "MODELSIM" OFF)
if(${MODELSIM})
    add_definitions(-DMODELSIM)
    MESSAGE( STATUS "Librorc is going to be compiled in simulation mode!")
endif(${MODELSIM})

#--- Add target for Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)

MESSAGE( STATUS "Install directory   = ${CMAKE_INSTALL_PREFIX}")

# Build flisock adapter only if MODELSIM is configured
IF(${MODELSIM})
    MESSAGE( STATUS "Adding (optional) FLI Socket Interface")
    add_subdirectory(fli)
ENDIF(${MODELSIM})

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tools_src)

#--- Setup CPack
include(CPack.txt)
INCLUDE(CPack)
