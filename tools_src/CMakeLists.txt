# Build rorctl
add_executable( rorctl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/rorctl.cpp)
target_link_libraries( rorctl rorc pthread ${EXTRA_LIBS})

# Build DMA test with Pattern Generator
add_executable( dma_in_pg
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_in_pg rorc pthread ${EXTRA_LIBS})

# Build DMA test DDL backend
add_executable( dma_in_ddl
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_in_ddl rorc pthread ${EXTRA_LIBS})

#-----------------------------------------------------------

# Build i2c
add_executable( i2c ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/i2c.cpp)
target_link_libraries( i2c rorc pthread ${EXTRA_LIBS})


# Build qsfpctrl
add_executable( qsfpctrl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/qsfpctrl.cpp)
target_link_libraries( qsfpctrl rorc pthread ${EXTRA_LIBS})


# Build refclkgenctrl
add_executable( refclkgenctrl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/refclkgenctrl.cpp)
target_link_libraries( refclkgenctrl rorc pthread ${EXTRA_LIBS})


# Build DMA buffer allocator
add_executable( crorc_preallocator
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/crorc_preallocator.cpp)
target_link_libraries( crorc_preallocator rorc pthread ${EXTRA_LIBS})


# Build DMA buffer stat tool
add_executable( bufferstats
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/bufferstats.cpp)
target_link_libraries( bufferstats rorc pthread ${EXTRA_LIBS})

# Build DMA monitor
add_executable( dma_monitor ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/dma_monitor.cpp)
target_link_libraries( dma_monitor rorc pthread ${EXTRA_LIBS})


# Build DMA-out testbed
add_executable( hlt_out_writer ${LIBRORC_SOURCE_DIR}/tools_src/hlt_out_writer/hlt_out_writer.cpp)
target_link_libraries( hlt_out_writer rorc pthread ${EXTRA_LIBS})


# Build gtxctrl
add_executable( gtxctrl ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/gtxctrl.cpp)
target_link_libraries( gtxctrl rorc pthread ${EXTRA_LIBS})


# Build dmactrl
add_executable( dmactrl ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/dmactrl.cpp)
target_link_libraries( dmactrl rorc pthread ${EXTRA_LIBS})


# Build ucctrl
add_executable( ucctrl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/ucctrl.cpp)
target_link_libraries( ucctrl rorc pthread ${EXTRA_LIBS})

# Build reset_error_counters
add_executable( reset_error_counters ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/reset_error_counters.cpp)
target_link_libraries( reset_error_counters rorc pthread ${EXTRA_LIBS})

# Build hwtests
add_executable( test_fmc ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/test_fmc.cpp)
target_link_libraries( test_fmc rorc pthread ${EXTRA_LIBS})

add_executable( ledctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ledctrl.cpp)
target_link_libraries( ledctrl rorc pthread ${EXTRA_LIBS})

add_executable( lvdsctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/lvdsctrl.cpp)
target_link_libraries( lvdsctrl rorc pthread ${EXTRA_LIBS})

add_executable( fanctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fanctrl.cpp)
target_link_libraries( fanctrl rorc pthread ${EXTRA_LIBS})

add_executable( ddr3_module_status ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ddr3_module_status.cpp)
target_link_libraries( ddr3_module_status rorc pthread ${EXTRA_LIBS})
