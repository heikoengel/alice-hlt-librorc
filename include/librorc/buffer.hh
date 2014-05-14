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

#ifndef LIBRORC_BUFFER_H
#define LIBRORC_BUFFER_H

#include "librorc/include_ext.hh"
#include "defines.hh"

#define LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED  1

//TODO: put this into an enum!
#define LIBRORC_DMA_FROM_DEVICE   2
#define LIBRORC_DMA_TO_DEVICE     1
#define LIBRORC_DMA_BIDIRECTIONAL 0

#define DMA_MODE 32

#if DMA_MODE==32
typedef struct
__attribute__((__packed__))
librorc_event_descriptor_struct
{
    volatile uint64_t offset;
    volatile uint32_t reported_event_size;
    volatile uint32_t calc_event_size;
    volatile uint64_t dummy;   /** do not use! */
    volatile uint64_t dummy2;  /** do not use! */
} librorc_event_descriptor;
#endif

typedef struct
librorc_sg_entry_struct
{
    uint64_t pointer;
    uint64_t length;
} librorc_sg_entry;


typedef struct PciDevice_struct        PciDevice;
typedef struct DMABuffer_struct        DMABuffer;
typedef struct DMABuffer_SGNode_struct DMABuffer_SGNode;




namespace LIBRARY_NAME
{

class device;
class dma_channel;
class buffer_sglist_programmer;

    /**
     * @class librorc::buffer
     * @brief Buffer management class.
     *        Instances of this class represent DMA buffers (report- and event
     *        buffer). Buffer IDs are unique and buffers can be mapped into
     *        different processes. Additionally, a buffer is not freed after
     *        deleting the related object, but detached from its persistent
     *        counterpart.
     **/
    class buffer
    {
        friend class dma_channel;
        friend class buffer_sglist_programmer;

        public:
             /**
              * Constructor, which allocates a completely new DMA buffer.
              * @param [in] dev
              *        Object, which represents the PCI device were the buffer
              *        is registered to. Please see device.hh for more information.
              * @param [in] size
              *        Size of the generated buffer.
              * @param [in] id
              *        Index of the buffer. Even IDs are report buffers, odd IDs are
              *        event buffers.
              * @param [in] overmap
              */
             buffer
             (
                 device   *dev,
                 uint64_t  size,
                 uint64_t  id,
                 int32_t   overmap,
                 int32_t   dma_direction
             );

             /**
              * Constructor, which attaches to an already allocated persistent DMA
              * buffer.
              * @param [in] dev
              *        Object, which represents the PCI device were the buffer
              *        is registered to. Please see device.hh for more information.
              * @param [in] id
              *        Index of the buffer. Even IDs are report buffers, odd IDs are
              *        event buffers.
              */
             buffer
             (
                 device   *dev,
                 uint64_t  id
             );

            ~buffer();

            /**
             * Free and release the attached buffer.
             * @return 0 on success, <0 on error (uses perror() )
             **/
            int32_t
            deallocate();

            /**
             * Get buffer index.
             * @return Buffer index.
             **/
            uint64_t
            getID()
            { return m_id; }

            /**
             * Get the overmapped flag of the buffer.
             * @return 0 if mapped normally, nonzero if wrap mapped.
             **/
            int32_t isOvermapped(); // TODO : boolean

            /**
             * Get physical Buffer size in bytes. Requested buffer
             * size from init() is rounded up to the next PAGE_SIZE
             * boundary.
             * @return Number of bytes allocated as buffer.
             **/
            uint64_t size()
            { return m_size; }

            /**
             * Get requested buffer size.
             * @return Number of bytes allocated as Buffer.
             */
            uint64_t
            getPhysicalSize()
            { return size(); }

            /**
             * Get the actually mapped memory region size.
             * @return Number of bytes allocated if not overmapped,
             *         twice the size if overmapped.
             */
            uint64_t
            getMappingSize()
            {
                if(isOvermapped() == 1)
                { return(2*size()); }
                return size();
            }

            /**
             * Get raw memory buffer.
             * @return Pointer to mmap'ed buffer memory.
             **/
            uint32_t*
            getMem()
            { return m_mem; }

            void clear();

            /**
             * Get number of scatter-gather entries for the Buffer
             * @return Number of sg-entries.
             **/
            uint64_t
            getnSGEntries()
            { return m_numberOfScatterGatherEntries; }

            /**
             * Get the scatter gather list for SG-DMA.
             * @return Vector of librorc_sg_entry.
             */
            std::vector<librorc_sg_entry>
            sgList()
            { return m_sglist_vector; }

            /**
             * Get the maximum number of report buffer entries in the RB
             * @return Maximum number of report buffer entries.
             **/
            uint64_t
            getMaxRBEntries()
            { return( size()/sizeof(librorc_event_descriptor) ); }


        /**
         * @internal
         */
        private:

            PciDevice        *m_device;
            DMABuffer        *m_buffer;
            DMABuffer_SGNode *m_sglist;

            std::vector<librorc_sg_entry> m_sglist_vector;

            uint32_t         *m_mem;
            uint64_t          m_id;
            int32_t           m_dmaDirection;
            uint64_t          m_numberOfScatterGatherEntries;
            uint64_t          m_size;

            pthread_mutex_t   m_mtx;

            DMABuffer*
            getPDABuffer()
            {
                return m_buffer;
            }

            /**
             * Connect to an existing buffer
             * @param dev parent librorc::device
             * @param id buffer ID of exisiting buffer
             * @return 0 on sucessful connect, -EPERM or -ENOMEM on errors
             **/
            void
            connect
            (
                device   *dev,
                uint64_t  id
            );


    };
}
#endif /** LIBRORC_BUFFER_H */
