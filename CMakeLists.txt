cmake_minimum_required(VERSION 2.8.9 FATAL_ERROR)
project(LIBRORC)

set( REQUIRED_PDA_VERSION "7:0:1" )

include_directories ( ${LIBRORC_SOURCE_DIR}/include )
set( CMAKE_CXX_FLAGS_DEBUG  "-Wall -O0 -g -DDEBUG" )
set( CMAKE_CXX_FLAGS "-Wall -g" )

MESSAGE( STATUS "-------------------------------------------------------------------------------")


#--- Find and check PDA -------------------------------------------------------
EXECUTE_PROCESS(
  COMMAND pda-config --include
  RESULT_VARIABLE PDA_RETURN
  OUTPUT_VARIABLE PDA_INCLUDE_DIR
  )

IF( PDA_RETURN )
  MESSAGE( STATUS "Failed to run pda-config because it is not in the path. ")
  MESSAGE( STATUS "Maybe the related module is not loaded? ")
  MESSAGE( STATUS "Try \"module load pda\" ...")
  MESSAGE( FATAL_ERROR "ABORT!!!")
ENDIF()
MESSAGE(STATUS "pda include         = ${PDA_INCLUDE_DIR}")

include_directories ( ${PDA_INCLUDE_DIR}  )

EXECUTE_PROCESS(
  COMMAND pda-config --ldlibrarypath
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE PDA_LD_LIBRARY_PATH
  )

MESSAGE(STATUS "pda LD_LIBRARY_PATH = ${PDA_LD_LIBRARY_PATH}")

EXECUTE_PROCESS(
  COMMAND pda-config --version
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE PDA_VERSION
  )

MESSAGE(STATUS "pda VERSION         = ${PDA_VERSION}")

IF( NOT(${REQUIRED_PDA_VERSION} STREQUAL ${PDA_VERSION}) )
  MESSAGE( STATUS "The wrong version of PDA found.")
  MESSAGE( STATUS "Found version ${PDA_VERSION} but require version ${REQUIRED_PDA_VERSION}.")
  MESSAGE( STATUS "Try \"module load pda/${REQUIRED_PDA_VERSION}\" ...")
  MESSAGE( FATAL_ERROR "ABORT!!!")
ENDIF()

find_library(PDA pda
    PATHS ${PDA_LD_LIBRARY_PATH})
set(EXTRA_LIBS ${EXTRA_LIBS} ${PDA})



#--- Compile LibRORC in Simulation mode ---------------------------------------
option(MyOption "SIM" OFF)
if(${SIM})
    add_definitions(-DSIM)
    MESSAGE( STATUS "Librorc is going to be compiled in simulation mode!")
endif(${SIM})


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

SET_TARGET_PROPERTIES( rorc PROPERTIES VERSION 0.0.0 SOVERSION 1)

# Build flisock adapter (if SIM is configured)
IF(${SIM})
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




MESSAGE( STATUS "-------------------------------------------------------------------------------")
