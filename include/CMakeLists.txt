PROJECT(librorc-devel)

SET( LIBRORC_HEADERS
  librorc/bar.hh
  librorc/bar_impl_sim.hh
  librorc/bar_impl_hw.hh
  librorc/buffer.hh
  librorc/datareplaychannel.hh
  librorc/ddl.hh
  librorc/defines.hh
  librorc/device.hh
  librorc/diu.hh
  librorc/dma_channel.hh
  librorc/event_sanity_checker.hh
  librorc/event_stream.hh
  librorc/eventfilter.hh
  librorc/fastclusterfinder.hh
  librorc/flash.hh
  librorc/gtx.hh
  librorc/high_level_event_stream.hh
  librorc/link.hh
  librorc/microcontroller.hh
  librorc/patterngenerator.hh
  librorc/refclk.hh
  librorc/registers.h
  librorc/siu.hh
  librorc/sysmon.hh
  librorc/ddr3.hh )

INSTALL( FILES librorc.h DESTINATION include)
INSTALL( FILES ${LIBRORC_HEADERS} DESTINATION include/librorc)
