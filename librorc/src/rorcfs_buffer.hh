/**
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
 * @date 2011-08-17
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _RORCLIB_RORCFS_BUFFER_H
#define _RORCLIB_RORCFS_BUFFER_H

#include "includes.hh"

#define DMA_MODE 32

#if DMA_MODE==32
struct
__attribute__((__packed__))
rorcfs_event_descriptor
{
    volatile uint64_t offset;
    volatile uint32_t reported_event_size;
    volatile uint32_t calc_event_size;
    volatile uint64_t dummy;   /** do not use! */
    volatile uint64_t dummy2;  /** do not use! */
};
#endif

//#if DMA_MODE==128
//struct rorcfs_event_descriptor
//{
//    uint64_t offset;
//    uint64_t length;
//    uint32_t reported_event_size;
//    uint32_t calc_event_size;
//    uint64_t dummy; /** do not use! */
//};
//#endif


typedef struct PciDevice_struct PciDevice;
typedef struct DMABuffer_struct DMABuffer;

/**
 * @class rorcfs_buffer
 * @brief buffer management class
 *
 * This class manages the DMA receive buffers. One instance of this
 * class represents one couple of EventBuffer and ReportBuffer with
 * their corresponding sysfs attributes
 **/
class rorcfs_buffer
{
    friend class rorcfs_dma_channel;

    public:
         rorcfs_buffer();
        ~rorcfs_buffer();

        /**
         * Allocate buffer: This function initiates allocation of an
         * EventBuffer of [size] bytes with Buffer-ID [id]. The size
         * of the according ReportBuffer is determined by the driver.
         * @param dev pointer to parent rorcfs_device instance
         * @param size Size of EventBuffer in bytes
         * @param id Buffer-ID to be used for this buffer. This ID has to
         *                   be unique within all instances of rorcfs_buffer on a
         *                   machine.
         * @param overmap enables overmapping of the physical pages if
         *                   nonzero
         * @param dma_direction select from RORCFS_DMA_FROM_DEVICE,
         *                   RORCFS_DMA_TO_DEVICE, RORCFS_DMA_BIDIRECTIONAL
         * @return 0 on sucess, -1 on error
         **/
        int32_t
        allocate
        (
            rorcfs_device *dev,
            uint64_t       size,
            uint64_t       id,
            int32_t        overmap,
            int32_t        dma_direction
        );

        /**
         * Free Buffer: This functions initiates de-allocation of the
         * attaced DMA buffers
         * @return 0 on sucess, <0 on error ( use perror() )
         **/
        int32_t
        deallocate();

        /**
         * Connect to an existing buffer
         * @param dev parent rorcfs device
         * @param id buffer ID of exisiting buffer
         * @return 0 on sucessful connect, -EPERM or -ENOMEM on errors
         **/
        int32_t
        connect
        (
            rorcfs_device *dev,
            uint64_t  id
        );

        /**
         * get Buffer-ID
         * @return Buffer-ID
         **/
        uint64_t
        getID()
        {
            return m_id;
        }

        /**
         * get the overmapped flag of the buffer
         * @return 0 if unset, nonzero if set
         **/
        int32_t isOvermapped(); // TODO : boolean

        /**
         * Get physical Buffer size in bytes. Requested buffer
         * size from init() is rounded up to the next PAGE_SIZE
         * boundary.
         * @return number of bytes allocated as Buffer
         **/
        uint64_t getSize();

        uint64_t
        getPhysicalSize()
        {
            return getSize();
        }

        uint64_t
        getMappingSize()
        {
            if(isOvermapped() == 1)
            {
                return(2*getSize());
            }
            return getSize();
        }

        /**
         * get memory buffer
         * @return pointer to mmap'ed buffer memory
         **/
        uint32_t *
        getMem()
        {
            return m_mem;
        }

        /**
         * Get number of scatter-gather entries for the Buffer
         * @return number of sg-entries
         **/
        uint64_t
        getnSGEntries()
        {
            return m_numberOfScatterGatherEntries;
        }

        /**
         * Get the maximum number of report buffer entries in the RB
         * @return maximum number of report buffer entries
         **/
        uint64_t
        getMaxRBEntries()
        {
            return (getSize()/sizeof(struct rorcfs_event_descriptor) );
        }


    private:

        PciDevice *m_device;
        DMABuffer *m_buffer;

        uint32_t  *m_mem;
        uint64_t   m_id;
        int32_t    m_dmaDirection;
        uint64_t   m_numberOfScatterGatherEntries;
        uint64_t   m_size;

        pthread_mutex_t  m_mtx;

        DMABuffer*
        getPDABuffer()
        {
            return m_buffer;
        }

};

#endif
