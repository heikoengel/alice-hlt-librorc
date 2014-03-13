SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Build rorctl
add_executable( rorctl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/rorctl.cpp)
target_link_libraries( rorctl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS rorctl RUNTIME DESTINATION bin )

# Build board_reset
add_executable( board_reset
  ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/board_reset.cpp)
target_link_libraries( board_reset rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS board_reset RUNTIME DESTINATION bin )

# Build ucctrl
add_executable( ucctrl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/ucctrl.cpp)
target_link_libraries( ucctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS ucctrl RUNTIME DESTINATION bin )

# Build reset_error_counters
add_executable( reset_error_counters ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/reset_error_counters.cpp)
target_link_libraries( reset_error_counters rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS reset_error_counters RUNTIME DESTINATION bin )

#-----------------------------------------------------------

# Build DMA test with Pattern Generator
#add_executable( dma_in_hwpg
#"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in.cpp"
#"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
#)
#target_link_libraries( dma_in_hwpg rorc pthread ${EXTRA_LIBS})
#INSTALL(TARGETS dma_in_hwpg RUNTIME DESTINATION bin )

# Build DMA test with Pattern Generator
add_executable( dma_in_hwpg_mc
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in_mc.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_in_hwpg_mc rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS dma_in_hwpg_mc RUNTIME DESTINATION bin )

# Build DMA test with DDL and FCF
add_executable( dma_in_ddl_fcf
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in_fcf.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_in_ddl_fcf rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS dma_in_ddl_fcf RUNTIME DESTINATION bin )

# Build DMA test DDL backend
#add_executable( dma_in_ddl
#"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in.cpp"
#"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
#)
#target_link_libraries( dma_in_ddl rorc pthread ${EXTRA_LIBS})
#INSTALL(TARGETS dma_in_ddl RUNTIME DESTINATION bin )

# Build DMA test pure backend
add_executable( dma_in
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_in.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_in rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS dma_in RUNTIME DESTINATION bin )

#-----------------------------------------------------------

# Build DMA-out testbed
add_executable( dma_out
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_out.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_out rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS dma_out RUNTIME DESTINATION bin )

# Build DMA-out testbed
add_executable( dma_out_mc
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_out_mc.cpp"
"${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp"
)
target_link_libraries( dma_out_mc rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS dma_out_mc RUNTIME DESTINATION bin )

#-----------------------------------------------------------

# Build i2c
add_executable( i2c ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/i2c.cpp)
target_link_libraries( i2c rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS i2c RUNTIME DESTINATION bin )

# Build qsfpctrl
add_executable( qsfpctrl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/qsfpctrl.cpp)
target_link_libraries( qsfpctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS qsfpctrl RUNTIME DESTINATION bin )

# Build refclkgenctrl
add_executable( refclkgenctrl ${LIBRORC_SOURCE_DIR}/tools_src/rorctl/refclkgenctrl.cpp)
target_link_libraries( refclkgenctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS refclkgenctrl RUNTIME DESTINATION bin )

# Build DMA buffer allocator
add_executable( crorc_preallocator
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/crorc_preallocator.cpp)
target_link_libraries( crorc_preallocator rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS crorc_preallocator RUNTIME DESTINATION bin )

# Build DMA buffer stat tool
add_executable( bufferstats
  ${LIBRORC_SOURCE_DIR}/tools_src/buffer_handling/bufferstats.cpp)
target_link_libraries( bufferstats rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS bufferstats RUNTIME DESTINATION bin )

# Build DMA monitor
add_executable( dma_monitor ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/dma_monitor.cpp)
target_link_libraries( dma_monitor rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS dma_monitor RUNTIME DESTINATION bin )

# Build PGDMA EventSize Sweeper
add_executable( hwpgdma_es_sweep ${LIBRORC_SOURCE_DIR}/tools_src/dma_monitor/hwpgdma_es_sweep.cpp)
target_link_libraries( hwpgdma_es_sweep rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS hwpgdma_es_sweep RUNTIME DESTINATION bin )

# Build gtxctrl
add_executable( gtxctrl ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/gtxctrl.cpp)
target_link_libraries( gtxctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS gtxctrl RUNTIME DESTINATION bin )

# Build ddlctrl
add_executable( ddlctrl ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/ddlctrl.cpp)
target_link_libraries( ddlctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS ddlctrl RUNTIME DESTINATION bin )

# Build diuctrl
add_executable( diuctrl ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/diuctrl.cpp)
target_link_libraries( diuctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS diuctrl RUNTIME DESTINATION bin )

# Build dmactrl
add_executable( dmactrl ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/dmactrl.cpp)
target_link_libraries( dmactrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS dmactrl RUNTIME DESTINATION bin )

# Build channeldump
add_executable( channeldump ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/channeldump.cpp)
target_link_libraries( channeldump rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS channeldump RUNTIME DESTINATION bin )

# Build ddr3_data_replay 
add_executable( ddr3_data_replay
  ${LIBRORC_SOURCE_DIR}/tools_src/linkctrl/ddr3_data_replay.cpp)
target_link_libraries( ddr3_data_replay rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS ddr3_data_replay RUNTIME DESTINATION bin )

#-----------------------------------------------------------

# Build hwtests
add_executable( fmc_loopback_tester
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fmc_loopback_tester.cpp)
target_link_libraries( fmc_loopback_tester rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS fmc_loopback_tester RUNTIME DESTINATION bin )

add_executable( ledctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ledctrl.cpp)
target_link_libraries( ledctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS ledctrl RUNTIME DESTINATION bin )

add_executable( lvdsctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/lvdsctrl.cpp)
target_link_libraries( lvdsctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS lvdsctrl RUNTIME DESTINATION bin )

add_executable( fanctrl ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fanctrl.cpp)
target_link_libraries( fanctrl rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS fanctrl RUNTIME DESTINATION bin )

add_executable( boardtest
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/crorc-smbus-ctrl.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest_modules.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/fmc_tester.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest.cpp )
target_link_libraries( boardtest rorc pthread pci ${EXTRA_LIBS})
INSTALL(TARGETS boardtest RUNTIME DESTINATION bin )

add_executable( board_to_board_test
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/board_to_board_test.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/board_to_board_test_modules.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/dma/dma_handling.cpp
  ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/boardtest_modules.cpp )
target_link_libraries( board_to_board_test rorc pthread pci ${EXTRA_LIBS})
INSTALL(TARGETS board_to_board_test RUNTIME DESTINATION bin )

add_executable( ddr3_module_status ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ddr3_module_status.cpp)
target_link_libraries( ddr3_module_status rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS ddr3_module_status RUNTIME DESTINATION bin )

add_executable( ddr3_tester_status ${LIBRORC_SOURCE_DIR}/tools_src/hwtest/ddr3_tester_status.cpp)
target_link_libraries( ddr3_tester_status rorc pthread ${EXTRA_LIBS})
INSTALL(TARGETS ddr3_tester_status RUNTIME DESTINATION bin )

#add_executable( ddr3_writer ${LIBRORC_SOURCE_DIR}/tools_src/sim_tests/ddr3_writer.cpp)
#target_link_libraries( ddr3_writer rorc pthread ${EXTRA_LIBS})
#INSTALL(TARGETS ddr3_writer RUNTIME DESTINATION bin )
