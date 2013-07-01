/**
 * @mainpage
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2011-08-16
 * @brief Library to interface to CRORCs
 *
 * @section Organization
 * A rorcfs_device object corresponds to a phsical RORC, resp. a PCI
 * device controlled by the rorcfs device driver. Use rorcfs_device::init()
 * to bind the object to the hardware.
 *
 * Objects of rorcfs_bar can be attached to a rorcfs_device to control
 * its PCIe Base Address Registers (BARs). Once the rorcfs_bar object is
 * initialized with rorcfs_bar::init() it can perform low level IO to/from
 * the RORC via rorcfs_bar::get() or rorcfs_bar::set() methods. These
 * methods are widely used in the rorcLib.
 *
 * A rorcfs_buffer object represents a DMA buffer on the host machine. This
 * buffers are mapped to the RORC for DMA transfers. Buffers can be
 * allocated and deallocated with rorcfs_buffer::allocate() and
 * rorcfs_buffer::deallocate(). The object can be bound to existing buffers
 * via the rorcfs_buffer::connect() method.
 *
 * All DMA operations are handled with objects of rorcfs_dma_channel. These
 * objects have to be initialized to a specific bar and offset within the
 * bar with rorcfs_dma_channel::init(). After initialization the DMA buffer
 * information can be copied to the RORC, the maximum payload can be set and
 * the DMA channel can be enabled. Once data is available it is written to the
 * buffers on the host machine.
 *
 * In order to receive data a DIU has to be instantiated. This is done with an
 * object of rorcfs_diu. The data source of the rorcfs_diu object can either
 * be an actual DIU or a pattern generator at the DIU Interface. The data source
 * can be selected with rorcfs_diu::setDataSource(). If a pattern generator is
 * chosen, an according rorcfs_pattern_generator object has to be attached to
 * the DIU in order to control the pattern generator parameters.
 **/


#ifndef LIBRORC_H
#define LIBRORC_H

    #include "sim_bar.hh"
    #include "librorc_registers.h"
    #include "librorc_defines.hh"
    #include "librorc_bar_proto.hh"
    #include "librorc_flash.hh"
    #include "librorc_sysmon.hh"
    #include "librorc_bar.hh"
    #include "librorc_device.hh"
    #include "librorc_buffer.hh"
    #include "librorc_dma_channel.hh"

#endif /** LIBRORC_H */
