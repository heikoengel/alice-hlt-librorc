PROJECT(librorc)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/kernelmodule/)

SET( LIBRORC_LIBRARY_SOURCE
  bar.cpp
  bar_impl_hw.cpp
  bar_impl_sim.cpp
  buffer.cpp
  datareplaychannel.cpp
  ddl.cpp
  ddr3.cpp
  device.cpp
  error.cpp
  diu.cpp
  dma_channel.cpp
  event_stream.cpp
  eventfilter.cpp
  fastclusterfinder.cpp
  flash.cpp
  gtx.cpp
  link.cpp
  microcontroller.cpp
  patterngenerator.cpp
  refclk.cpp
  siu.cpp
  sysmon.cpp
  sysfs_handler.cpp
  )

ADD_LIBRARY(rorc SHARED ${LIBRORC_LIBRARY_SOURCE})
TARGET_LINK_LIBRARIES( rorc pthread pci ${EXTRA_LIBS})
SET_TARGET_PROPERTIES( rorc PROPERTIES VERSION ${LIBRORC_VERSION} SOVERSION ${LIBRORC_MAJOR_VERSION})
INSTALL(TARGETS rorc LIBRARY DESTINATION lib)
