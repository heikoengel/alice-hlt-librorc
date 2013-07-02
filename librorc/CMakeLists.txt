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

EXECUTE_PROCESS(
  COMMAND pda-config --include
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE PDA_INCLUDE_DIR
  )

MESSAGE(STATUS "--------------")
MESSAGE(STATUS "pda include = ${PDA_INCLUDE_DIR}")
MESSAGE(STATUS "--------------")

include_directories ( ${LIBRORC_SOURCE_DIR}/src )
include_directories ( ${LIBRORC_SOURCE_DIR}/include )
include_directories ( ${PDA_INCLUDE_DIR}  )

add_definitions(-DDEBUG)
add_definitions(-DSIM)

#add_library ( LIBRORC_OBJECTS OBJECT  ${LIBRARY_FILES} )
#add_library ( rorc STATIC SHARED $<TARGET_OBJECTS:LIBRORC_OBJECTS> )
#add_library ( rorc SHARED $<TARGET_OBJECTS:LIBRORC_OBJECTS> )

add_library ( rorc SHARED  ${LIBRARY_FILES} )


#add_executable (helloDemo demo.cxx demo_b.cxx)
#target_link_libraries (helloDemo Hello) 
