SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#-----------------------------------------------------------

# Build all in rorctl
SET( RORCTL_LIST crorc_i2c qsfpctrl refclkgenctrl ucctrl rorctl crorc_reset
  crorc_sensors crorc_status_dump crorc_flash crorc_ddr3ctrl )
FOREACH( STEMNAME ${RORCTL_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
  INSTALL( TARGETS ${STEMNAME} RUNTIME DESTINATION bin )
ENDFOREACH( STEMNAME )

# Build all in linkctrl
SET( RORCTL_LIST gtxctrl ddlctrl diuctrl dmactrl ddr3_data_replay )
FOREACH( STEMNAME ${RORCTL_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

#-----------------------------------------------------------

add_executable( crorc_fpga_ctrl
  "${LIBRORC_SOURCE_DIR}/tools_src/crorc_fpga_ctrl/class_crorc.cpp"
  "${LIBRORC_SOURCE_DIR}/tools_src/crorc_fpga_ctrl/crorc_fpga_ctrl.cpp"
  )
target_link_libraries( crorc_fpga_ctrl rorc ${EXTRA_LIBS} )
INSTALL(TARGETS crorc_fpga_ctrl RUNTIME DESTINATION bin )

# Build DMA-in standalone tool
add_executable( dma_in
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_in rorc ${EXTRA_LIBS})

# Build DMA-out standalone-tool
add_executable( dma_out
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_out.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_out rorc ${EXTRA_LIBS})

# DMA test apps, mainly for simulation purposes
SET( DMA_TEST_LIST dma_in_mc dma_in_ddr3_mc dma_out_mc )
FOREACH( STEMNAME ${DMA_TEST_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    ${LIBRORC_SOURCE_DIR}/tools_src/dma/${STEMNAME}.cpp
    ${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp
  )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

#-----------------------------------------------------------

# Build all in buffer_handling
SET( BUFFER_HANDLING_LIST crorc_preallocator bufferstats buffer_alloc_test
  buffer_access_test reportbufferdump )
FOREACH( STEMNAME ${BUFFER_HANDLING_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

# Build DMA monitor
add_executable( dma_monitor ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/dma_monitor.cpp)
target_link_libraries( dma_monitor rorc ${EXTRA_LIBS})

# Build PGDMA EventSize Sweeper
add_executable( hwpgdma_es_sweep ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/hwpgdma_es_sweep.cpp)
target_link_libraries( hwpgdma_es_sweep rorc ${EXTRA_LIBS})

#-----------------------------------------------------------

# Build hwtests
SET( HWTEST_LIST fmc_loopback_tester ledctrl lvdsctrl fanctrl
  ddr3_module_status ddr3_tester_status )
FOREACH( STEMNAME ${HWTEST_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

add_executable( boardtest
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/crorc-smbus-ctrl.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest_modules.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fmc_tester.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest.cpp )
target_link_libraries( boardtest rorc ${EXTRA_LIBS})

add_executable( board_to_board_test
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/board_to_board_test.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/board_to_board_test_modules.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest_modules.cpp )
target_link_libraries( board_to_board_test rorc ${EXTRA_LIBS})
