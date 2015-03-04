/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

#ifndef LIBRORC_BUFFER_H
#define LIBRORC_BUFFER_H

#include <vector>
#include <librorc/defines.hh>

#define LIBRORC_BUFFER_ERROR_INVALID_SIZE     1
#define LIBRORC_BUFFER_ERROR_GETLENGTH_FAILED 2
#define LIBRORC_BUFFER_ERROR_GETMAP_FAILED    3
#define LIBRORC_BUFFER_ERROR_GETLIST_FAILED   4
#define LIBRORC_BUFFER_ERROR_DELETE_FAILED    5
#define LIBRORC_BUFFER_ERROR_ALLOC_FAILED     6
#define LIBRORC_BUFFER_ERROR_WRAPMAP_FAILED   7
#define LIBRORC_BUFFER_ERROR_GETBUF_FAILED    8
#define LIBRORC_BUFFER_ERROR_GETMAPTWO_FAILED 9


typedef struct PciDevice_struct        PciDevice;
typedef struct DMABuffer_struct        DMABuffer;
typedef struct DMABuffer_SGNode_struct DMABuffer_SGNode;


namespace LIBRARY_NAME
{

typedef struct
ScatterGatherEntryStruct
{
    uint64_t pointer;
    uint64_t length;
} ScatterGatherEntry;

typedef struct
__attribute__((__packed__))
EventDescriptorStruct
{
    volatile uint64_t offset;
    volatile uint32_t reported_event_size;
    volatile uint32_t calc_event_size;
    volatile uint64_t dummy;   /** do not use! */
    volatile uint64_t dummy2;  /** do not use! */
} EventDescriptor;

static errmsg_t buffer_errmsg_table[] = {
    {LIBRORC_BUFFER_ERROR_INVALID_SIZE, "requested buffer with invalid size"},
    {LIBRORC_BUFFER_ERROR_GETLENGTH_FAILED, "failed to get buffer size"},
    {LIBRORC_BUFFER_ERROR_GETMAP_FAILED, "failed to get buffer map"},
    {LIBRORC_BUFFER_ERROR_GETLIST_FAILED, "failed to get scatter-gather-list"},
    {LIBRORC_BUFFER_ERROR_DELETE_FAILED, "failed to delete buffer"},
    {LIBRORC_BUFFER_ERROR_ALLOC_FAILED, "failed to allocate buffer"},
    {LIBRORC_BUFFER_ERROR_WRAPMAP_FAILED, "failed to overmap buffer"},
    {LIBRORC_BUFFER_ERROR_GETBUF_FAILED, "failed to find buffer"},
    {LIBRORC_BUFFER_ERROR_GETMAPTWO_FAILED, "failed to get wrap-map state"}};
const ssize_t buffer_errmsg_table_len = sizeof(buffer_errmsg_table) / sizeof(errmsg_t);

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
                 int32_t   overmap
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
                 uint64_t  id,
                 int32_t   overmap
             );

            ~buffer();

            /**
             * Free and release the attached buffer.
             * @return 0 on success, <0 on error (uses perror() )
             **/
            int32_t deallocate();

            /**
             * Get buffer index.
             * @return Buffer index.
             **/
            uint64_t
            getID()
            { return m_id; }

            /**
             * Get the overmapped flag of the buffer.
             * @return false if mapped normally, true if wrap mapped.
             **/
            bool isOvermapped();

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
                if(isOvermapped())
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
            std::vector<ScatterGatherEntry>
            sgList()
            { return m_sglist_vector; }

            /**
             * Get the maximum number of report buffer entries in the RB
             * @return Maximum number of report buffer entries.
             **/
            uint64_t
            getMaxRBEntries()
            { return( size()/sizeof(EventDescriptor) ); }


            /**
             * resolv buffer offset to physical DMA address
             * @param offset buffer offset to be resolved
             * @param phys_addr pointer to uint64_t to save the physical
             * address
             * @param rem_sg_length pointer to uint64_t to save the
             * remaining length of the currently addressed scatter gather
             * segment
             * @return true on success, false if the offset was not found in the buffer
             **/
            bool
            offsetToPhysAddr
            (
                uint64_t offset,
                uint64_t *phys_addr,
                uint64_t *rem_sg_length
            );

            /**
             * resolv a physical DMA address to buffer offset
             * @param phys_addr physical address to be resolved
             * @param offset pointer to uint64_t to save the buffer offset
             * @return true on success, false if the physical address was not
             * found in the buffer
             **/
            bool
            physAddrToOffset
            (
                uint64_t phys_addr,
                uint64_t *offset
            );

            /**
             * compose a scatter-gather list of physical start addresses
             * and lengths for a buffer segment defined by offset and size
             * @param offset starting offset within the buffer
             * @param size size of the segment
             * @param list pointer to a vector list into which the resulting
             * scatter-gather list is stored
             * @return true on success, false on error
             **/
            bool
            composeSglistFromBufferSegment
            (
                 uint64_t offset,
                 uint64_t size,
                 std::vector<ScatterGatherEntry> *list
            );

            const char* errMsg(int err) { return _errMsg(buffer_errmsg_table, buffer_errmsg_table_len, err); }

        /**
         * @internal
         */
        private:

            PciDevice        *m_device;
            DMABuffer        *m_buffer;
            DMABuffer_SGNode *m_sglist;

            std::vector<ScatterGatherEntry> m_sglist_vector;

            uint32_t         *m_mem;
            uint64_t          m_id;
            uint64_t          m_numberOfScatterGatherEntries;
            uint64_t          m_size;

            //pthread_mutex_t   m_mtx;

            DMABuffer*
            getPDABuffer()
            {
                return m_buffer;
            }

            /**
             * Connect to an existing buffer.
             * can throw exceptions.
             **/
            void connect();


    };
}
#endif /** LIBRORC_BUFFER_H */
