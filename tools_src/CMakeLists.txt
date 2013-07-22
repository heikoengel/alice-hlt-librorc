# Build rorctl
add_executable( rorctl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/rorctl.cpp)
target_link_libraries( rorctl rorc pthread ${EXTRA_LIBS})

# Build i2c
add_executable( i2c ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/i2c.cpp)
target_link_libraries( i2c rorc pthread ${EXTRA_LIBS})

# Build DMA buffer allocator
add_executable( crorc_preallocator
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/crorc_preallocator.cpp)
target_link_libraries( crorc_preallocator rorc pthread ${EXTRA_LIBS})

# Build DMA buffer stat tool
add_executable( bufferstats
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/bufferstats.cpp)
target_link_libraries( bufferstats rorc pthread ${EXTRA_LIBS})

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
