set( LIBRORC_LIBRARY_SOURCE
"${LIBRORC_SOURCE_DIR}/src/device.cpp"
"${LIBRORC_SOURCE_DIR}/src/bar.cpp"
"${LIBRORC_SOURCE_DIR}/src/buffer.cpp"
"${LIBRORC_SOURCE_DIR}/src/device.cpp"
"${LIBRORC_SOURCE_DIR}/src/dma_channel.cpp"
"${LIBRORC_SOURCE_DIR}/src/link.cpp"
"${LIBRORC_SOURCE_DIR}/src/flash.cpp"
"${LIBRORC_SOURCE_DIR}/src/sysmon.cpp"
"${LIBRORC_SOURCE_DIR}/src/refclk.cpp"
"${LIBRORC_SOURCE_DIR}/src/microcontroller.cpp"
"${LIBRORC_SOURCE_DIR}/src/sim_bar.cpp"
"${LIBRORC_SOURCE_DIR}/src/event_stream.cpp"
"${LIBRORC_SOURCE_DIR}/src/event_sanity_checker.cpp"
"${LIBRORC_SOURCE_DIR}/src/patterngenerator.cpp"
"${LIBRORC_SOURCE_DIR}/src/fastclusterfinder.cpp"
"${LIBRORC_SOURCE_DIR}/src/datareplaychannel.cpp"
"${LIBRORC_SOURCE_DIR}/src/ddl.cpp"
"${LIBRORC_SOURCE_DIR}/src/diu.cpp"
"${LIBRORC_SOURCE_DIR}/src/siu.cpp"
"${LIBRORC_SOURCE_DIR}/src/eventfilter.cpp"
"${LIBRORC_SOURCE_DIR}/src/event_generator.cpp"
)

add_library(LIBRORC_OBJECTS OBJECT ${LIBRORC_LIBRARY_SOURCE})
set_target_properties( LIBRORC_OBJECTS PROPERTIES POSITION_INDEPENDENT_CODE ON )
