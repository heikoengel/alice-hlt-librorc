cmake_minimum_required(VERSION 2.8.9)
project(LIBRORC)

include_directories ( ${LIBRORC_SOURCE_DIR}/include )
set( CMAKE_CXX_FLAGS_DEBUG  "-Wall -O0 -g -DDEBUG" )


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
MESSAGE( STATUS "pda include = ${PDA_INCLUDE_DIR}")

include_directories ( ${PDA_INCLUDE_DIR}  )

EXECUTE_PROCESS(
  COMMAND pda-config --ldlibrarypath
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE PDA_LD_LIBRARY_PATH
  )

MESSAGE(STATUS "pda LD_LIBRARY_PATH = ${PDA_LD_LIBRARY_PATH}")

find_library(PDA pda
    PATHS ${PDA_LD_LIBRARY_PATH})
set(EXTRA_LIBS ${EXTRA_LIBS} ${PDA})


#--- Compile LibRORC in Simulation mode ---------------------------------------
option(MyOption "SIM" OFF)
if(${SIM})
    add_definitions(-DSIM)
    MESSAGE( STATUS "Librorc is going to be compiled in simulation mode!")
endif(${SIM})


#--- add target for Doxygen ---------------------------------------------------
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
# Build librorc add STATIC / SHARED here if needed
add_library( rorc SHARED $<TARGET_OBJECTS:LIBRORC_OBJECTS> )

# Build flisock adapter (if SIM is configured)
IF(${SIM})
    MESSAGE( STATUS "Adding (optional) FLI Socket Interface")
#    add_library( fli SHARED ${FLISOCK_LIBRARY_SOURCE} )
    add_library( fli SHARED $<TARGET_OBJECTS:FLI_OBJECTS> )
    set_target_properties(fli PROPERTIES PREFIX "")
ENDIF()


MESSAGE( STATUS "-------------------------------------------------------------------------------")


# Build rorctl
add_executable( rorctl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/rorctl.cpp)
target_link_libraries( rorctl rorc pthread ${EXTRA_LIBS})

# Build DMA buffer allocator
add_executable( crorc_preallocator ${LIBRORC_SOURCE_DIR}/tools_src/crorc_preallocator/crorc_preallocator.cpp)
target_link_libraries( crorc_preallocator rorc pthread ${EXTRA_LIBS})

# Build DMA test with pattern generator
add_executable( pgdma_continuous ${LIBRORC_SOURCE_DIR}/tools_src/dma/pgdma_continuous.cpp)
target_link_libraries( pgdma_continuous rorc pthread ${EXTRA_LIBS})

# Build DMA test
add_executable( gtxdma_continuous ${LIBRORC_SOURCE_DIR}/tools_src/dma/gtxdma_continuous.cpp)
target_link_libraries( gtxdma_continuous rorc pthread ${EXTRA_LIBS})

# Build DMA monitor
add_executable( dma_monitor ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/dma_monitor.cpp)
target_link_libraries( dma_monitor rorc pthread ${EXTRA_LIBS})

# Build DMA-out testbed
add_executable( hlt_out_writer ${LIBRORC_SOURCE_DIR}/tools_src/hlt_out_writer/hlt_out_writer.cpp)
target_link_libraries( hlt_out_writer rorc pthread ${EXTRA_LIBS})