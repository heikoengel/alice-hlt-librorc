PROJECT(librorc-tools)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

# Build all in rorctl
SET( RORCTL_LIST qsfpctrl refclkgenctrl ucctrl rorctl )
FOREACH( STEMNAME ${RORCTL_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    rorctl/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
  #INSTALL( TARGETS ${STEMNAME} RUNTIME DESTINATION bin )
ENDFOREACH( STEMNAME )

# Build all in linkctrl
SET( RORCTL_LIST gtxctrl ddlctrl diuctrl dmactrl ddr3_data_replay )
FOREACH( STEMNAME ${RORCTL_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    linkctrl/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

#-----------------------------------------------------------

# Build DMA-in standalone tool
add_executable( dma_in
"dma/dma_in.cpp"
"dma/dma_handling.cpp"
)
target_link_libraries( dma_in rorc ${EXTRA_LIBS})

# Build DMA-out standalone-tool
add_executable( dma_out
"dma/dma_out.cpp"
"dma/dma_handling.cpp"
)
target_link_libraries( dma_out rorc ${EXTRA_LIBS})

# DMA test apps, mainly for simulation purposes
SET( DMA_TEST_LIST dma_in_mc dma_in_ddr3_mc dma_out_mc )
FOREACH( STEMNAME ${DMA_TEST_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    dma/${STEMNAME}.cpp
    dma/dma_handling.cpp
  )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

#-----------------------------------------------------------

# Build all in buffer_handling
SET( BUFFER_HANDLING_LIST crorc_preallocator bufferstats buffer_alloc_test
  buffer_access_test reportbufferdump )
FOREACH( STEMNAME ${BUFFER_HANDLING_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    buffer_handling/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

# Build DMA monitor
add_executable( dma_monitor dma_monitor/dma_monitor.cpp)
target_link_libraries( dma_monitor rorc ${EXTRA_LIBS})

# Build PGDMA EventSize Sweeper
add_executable( hwpgdma_es_sweep dma_monitor/hwpgdma_es_sweep.cpp)
target_link_libraries( hwpgdma_es_sweep rorc ${EXTRA_LIBS})

#-----------------------------------------------------------

# Build all in hwtest
SET( HWTEST_LIST fmc_loopback_tester ledctrl lvdsctrl fanctrl
  ddr3_module_status ddr3_tester_status )
FOREACH( STEMNAME ${HWTEST_LIST} )
  ADD_EXECUTABLE( ${STEMNAME}
    hwtest/${STEMNAME}.cpp )
  TARGET_LINK_LIBRARIES( ${STEMNAME} rorc ${EXTRA_LIBS} )
ENDFOREACH( STEMNAME )

add_executable( boardtest
  hwtest/crorc-smbus-ctrl.cpp
  hwtest/boardtest_modules.cpp
  hwtest/fmc_tester.cpp
  dma/dma_handling.cpp
  hwtest/boardtest.cpp )
target_link_libraries( boardtest rorc ${EXTRA_LIBS})

add_executable( board_to_board_test
  hwtest/board_to_board_test.cpp
  hwtest/board_to_board_test_modules.cpp
  dma/dma_handling.cpp
  hwtest/boardtest_modules.cpp )
target_link_libraries( board_to_board_test rorc ${EXTRA_LIBS})
