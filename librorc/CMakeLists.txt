set ( LIBRARY_FILES
"${LIBRORC_SOURCE_DIR}/src/flisock.c"
"${LIBRORC_SOURCE_DIR}/src/librorc_device.cpp"
"${LIBRORC_SOURCE_DIR}/src/librorc_bar.cpp"
"${LIBRORC_SOURCE_DIR}/src/librorc_buffer.cpp"
"${LIBRORC_SOURCE_DIR}/src/librorc_device.cpp"
"${LIBRORC_SOURCE_DIR}/src/librorc_dma_channel.cpp"
"${LIBRORC_SOURCE_DIR}/src/librorc_flash.cpp"
"${LIBRORC_SOURCE_DIR}/src/librorc_sysmon.cpp"
"${LIBRORC_SOURCE_DIR}/src/sim_bar.cpp"
)

cmake_minimum_required(VERSION 2.8)
project(LIBRORC)

include_directories ( ${LIBRORC_SOURCE_DIR}/include )
set ( CMAKE_CXX_FLAGS_DEBUG  "-Wall -O0 -g -DDEBUG" )


EXECUTE_PROCESS(
  COMMAND pda-config --include
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE PDA_INCLUDE_DIR
  )

include_directories ( ${PDA_INCLUDE_DIR}  )

MESSAGE(STATUS "--------------")
MESSAGE(STATUS "pda include = ${PDA_INCLUDE_DIR}")
MESSAGE(STATUS "--------------")


option(MyOption "SIM" OFF)
if(${SIM})
    add_definitions(-DSIM)
    MESSAGE(STATUS "--------------")
    MESSAGE(STATUS "Librorc is going to be compiled in simulation mode!")
    MESSAGE(STATUS "--------------")
endif(${SIM})



find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)


add_library ( rorc SHARED  ${LIBRARY_FILES} )


#add_executable (helloDemo demo.cxx demo_b.cxx)
#target_link_libraries (helloDemo Hello) 
