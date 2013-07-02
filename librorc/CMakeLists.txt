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

include_directories ( ${LIBRORC_SOURCE_DIR}/src )
include_directories ( ${LIBRORC_SOURCE_DIR}/include )
include_directories ( `pda-config --include` )
include_directories ( /opt/packages/pda/5.0.0/include/ )

add_library ( rorc ${LIBRARY_FILES} )



#add_executable (helloDemo demo.cxx demo_b.cxx)
#target_link_libraries (helloDemo Hello) 
