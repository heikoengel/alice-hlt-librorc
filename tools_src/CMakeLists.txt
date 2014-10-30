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
  INSTALL( TARGETS ${STEMNAME} RUNTIME DESTINATION bin )
ENDFOREACH( STEMNAME )

#-----------------------------------------------------------

add_executable( crorc_fpga_ctrl
  "${LIBRORC_SOURCE_DIR}/tools_src/crorc_fpga_ctrl/class_crorc.cpp"
  "${LIBRORC_SOURCE_DIR}/tools_src/crorc_fpga_ctrl/crorc_fpga_ctrl.cpp"
  )
target_link_libraries( crorc_fpga_ctrl rorc ${EXTRA_LIBS} )


# Build DMA-in standalone tool
add_executable( dma_in
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_in rorc ${EXTRA_LIBS})
#INSTALL(TARGETS dma_in RUNTIME DESTINATION bin )

# Build DMA-out standalone-tool
add_executable( dma_out
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_out.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_out rorc ${EXTRA_LIBS})
#INSTALL(TARGETS dma_out RUNTIME DESTINATION bin )

# Build DMA-out testbed
#add_executable( dma_out_mc
#"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_out_mc.cpp"
#"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
#)
#target_link_libraries( dma_out_mc rorc ${EXTRA_LIBS})
#INSTALL(TARGETS dma_out_mc RUNTIME DESTINATION bin )

#-----------------------------------------------------------

# Build DMA buffer allocator
add_executable( crorc_preallocator
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/crorc_preallocator.cpp)
target_link_libraries( crorc_preallocator rorc ${EXTRA_LIBS})
#INSTALL(TARGETS crorc_preallocator RUNTIME DESTINATION bin )

# Build DMA buffer stat tool
add_executable( bufferstats
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/bufferstats.cpp)
target_link_libraries( bufferstats rorc ${EXTRA_LIBS})
INSTALL(TARGETS bufferstats RUNTIME DESTINATION bin )

# Build DMA buffer stat tool
add_executable( buffer_alloc_test
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/buffer_alloc_test.cpp)
target_link_libraries( buffer_alloc_test rorc ${EXTRA_LIBS})
#INSTALL(TARGETS buffer_alloc_test RUNTIME DESTINATION bin )

add_executable( buffer_access_test
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/buffer_access_test.cpp)
target_link_libraries( buffer_access_test rorc ${EXTRA_LIBS})
#INSTALL(TARGETS buffer_access_test RUNTIME DESTINATION bin )

# Build DMA monitor
add_executable( dma_monitor ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/dma_monitor.cpp)
target_link_libraries( dma_monitor rorc ${EXTRA_LIBS})
#INSTALL(TARGETS dma_monitor RUNTIME DESTINATION bin )

# Build PGDMA EventSize Sweeper
add_executable( hwpgdma_es_sweep ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/hwpgdma_es_sweep.cpp)
target_link_libraries( hwpgdma_es_sweep rorc ${EXTRA_LIBS})
#INSTALL(TARGETS hwpgdma_es_sweep RUNTIME DESTINATION bin )

#-----------------------------------------------------------

# Build hwtests
add_executable( fmc_loopback_tester
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fmc_loopback_tester.cpp)
target_link_libraries( fmc_loopback_tester rorc ${EXTRA_LIBS})
#INSTALL(TARGETS fmc_loopback_tester RUNTIME DESTINATION bin )

add_executable( ledctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ledctrl.cpp)
target_link_libraries( ledctrl rorc ${EXTRA_LIBS})
#INSTALL(TARGETS ledctrl RUNTIME DESTINATION bin )

add_executable( lvdsctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/lvdsctrl.cpp)
target_link_libraries( lvdsctrl rorc ${EXTRA_LIBS})
#INSTALL(TARGETS lvdsctrl RUNTIME DESTINATION bin )

add_executable( fanctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fanctrl.cpp)
target_link_libraries( fanctrl rorc ${EXTRA_LIBS})
#INSTALL(TARGETS fanctrl RUNTIME DESTINATION bin )

add_executable( boardtest
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/crorc-smbus-ctrl.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest_modules.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fmc_tester.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest.cpp )
target_link_libraries( boardtest rorc ${EXTRA_LIBS})
#INSTALL(TARGETS boardtest RUNTIME DESTINATION bin )

add_executable( board_to_board_test
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/board_to_board_test.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/board_to_board_test_modules.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest_modules.cpp )
target_link_libraries( board_to_board_test rorc ${EXTRA_LIBS})
#INSTALL(TARGETS board_to_board_test RUNTIME DESTINATION bin )

add_executable( ddr3_module_status ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ddr3_module_status.cpp)
target_link_libraries( ddr3_module_status rorc ${EXTRA_LIBS})
INSTALL(TARGETS ddr3_module_status RUNTIME DESTINATION bin )

add_executable( ddr3_tester_status ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ddr3_tester_status.cpp)
target_link_libraries( ddr3_tester_status rorc ${EXTRA_LIBS})
#INSTALL(TARGETS ddr3_tester_status RUNTIME DESTINATION bin )
