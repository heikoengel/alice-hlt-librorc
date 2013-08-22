set( LIBRORC_LIBRARY_SOURCE
"${LIBRORC_SOURCE_DIR}/src/device.cpp"
"${LIBRORC_SOURCE_DIR}/src/bar.cpp"
"${LIBRORC_SOURCE_DIR}/src/buffer.cpp"
"${LIBRORC_SOURCE_DIR}/src/device.cpp"
"${LIBRORC_SOURCE_DIR}/src/dma_channel.cpp"
"${LIBRORC_SOURCE_DIR}/src/flash.cpp"
"${LIBRORC_SOURCE_DIR}/src/sysmon.cpp"
"${LIBRORC_SOURCE_DIR}/src/sim_bar.cpp"
"${LIBRORC_SOURCE_DIR}/src/event_stream.cpp"
)

add_library(LIBRORC_OBJECTS OBJECT ${LIBRORC_LIBRARY_SOURCE})
set_target_properties( LIBRORC_OBJECTS PROPERTIES POSITION_INDEPENDENT_CODE ON )
