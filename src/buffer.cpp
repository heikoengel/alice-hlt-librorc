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

#ifdef PDA
#include <pda.h>
#endif
#include <vector>
#include <sys/mman.h>
#include <string.h>

#include <librorc/buffer.hh>
#include <librorc/device.hh>
#include <librorc/error.hh>
#include <librorc/sysfs_handler.hh>

namespace LIBRARY_NAME
{

buffer::buffer
(
    device   *dev,
    ssize_t   size,
    uint64_t  id,
    int32_t   overmap
)
{
    m_id      = id;
    m_sglist_vector.clear();

#ifdef PDA
    m_device            = dev->getPdaPciDevice();

    bool newAllocNeeded = false;
    m_buffer            = NULL;

    if( size == 0 )
    { throw LIBRORC_BUFFER_ERROR_INVALID_SIZE; }

    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    { newAllocNeeded = true; }
    else
    {
        size_t length = 0;
        if(DMABuffer_getLength(m_buffer, &length) != PDA_SUCCESS)
        { throw LIBRORC_BUFFER_ERROR_GETLENGTH_FAILED; }

        if(size != length)
        {
            if(PciDevice_deleteDMABuffer(m_device, m_buffer) != PDA_SUCCESS)
            { throw LIBRORC_BUFFER_ERROR_DELETE_FAILED; }
            newAllocNeeded = true;
        }
    }

    if(newAllocNeeded)
    {
        if
        (
            PDA_SUCCESS !=
                PciDevice_allocDMABuffer
                    (m_device, id, size, &m_buffer)
        )
        { throw LIBRORC_BUFFER_ERROR_ALLOC_FAILED; }
    }

    if(overmap == 1)
    {
        if(!isOvermapped()) {
            if(PDA_SUCCESS != DMABuffer_wrapMap(m_buffer) )
            { throw LIBRORC_BUFFER_ERROR_WRAPMAP_FAILED; }
        }
    }

#else

    m_free_hdl = false;
    m_hdl = dev->getHandler();
    if (m_hdl->buffer_exists(m_id)) {
      m_size = m_hdl->get_buffer_size(m_id);
    } else {
      m_size = 0;
    }
    m_wrapmap = (overmap==1);
    bool deallocate = false;
    bool allocate = true;

    if (m_size > 0) {
      if (m_size != size) {
          deallocate = true;
          allocate = true;
      } else {
          deallocate = false;
          allocate = false;
      }
    }

    if (deallocate) {
      if (m_hdl->deallocate_buffer(m_id) < 0) {
        throw LIBRORC_BUFFER_ERROR_DELETE_FAILED;
      }
    }
    if (allocate) {
      if (m_hdl->allocate_buffer(m_id, size) < 0) {
        throw LIBRORC_BUFFER_ERROR_ALLOC_FAILED;
      }
      m_size = size;
    }

#endif
    connect();
}



buffer::buffer
(
    device   *dev,
    uint64_t  id,
    int32_t   overmap
)
{
    m_id      = id;
    m_sglist_vector.clear();

#ifdef PDA
    m_device = dev->getPdaPciDevice();

    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    { throw LIBRORC_BUFFER_ERROR_GETBUF_FAILED; }

    if(overmap == 1 && !isOvermapped())
    {
        if(PDA_SUCCESS != DMABuffer_wrapMap(m_buffer) )
        { throw LIBRORC_BUFFER_ERROR_WRAPMAP_FAILED; }
    }
#else
    m_free_hdl = false;
    m_hdl = dev->getHandler();
    m_size = m_hdl->get_buffer_size(m_id);
    m_wrapmap = (overmap==1);
#endif
    connect();
}

#ifndef PDA
buffer::buffer
(
  uint64_t deviceId,
  uint64_t bufferId,
  int32_t overmap
)
{
    m_id = bufferId;
    m_free_hdl = true;
    m_sglist_vector.clear();
    m_hdl = new sysfs_handler(deviceId, LIBRORC_SCANMODE_UIO);
    m_size = m_hdl->get_buffer_size(m_id);
    m_wrapmap = (overmap==1);
    connect();
}
#endif


void
buffer::connect()

{
#ifdef PDA
    m_size = 0;
    m_mem = NULL;

    size_t size;
    if( DMABuffer_getLength( m_buffer, &size) != PDA_SUCCESS )
    { throw LIBRORC_BUFFER_ERROR_GETLENGTH_FAILED; }
    m_size = size;

    if( DMABuffer_getMap(m_buffer, (void**)(&m_mem) )!=PDA_SUCCESS )
    { throw LIBRORC_BUFFER_ERROR_GETMAP_FAILED; }

    m_sglist = NULL;
    if(DMABuffer_getSGList(m_buffer, &m_sglist)!=PDA_SUCCESS)
    { throw LIBRORC_BUFFER_ERROR_GETLIST_FAILED; }

    m_numberOfScatterGatherEntries = 0;
    for(DMABuffer_SGNode *sg=m_sglist; sg!=NULL; sg=sg->next)
    { m_numberOfScatterGatherEntries++; }

    for(DMABuffer_SGNode *sg=m_sglist; sg!=NULL; sg=sg->next)
    {
        ScatterGatherEntry entry;
        entry.pointer = (uint64_t)sg->d_pointer,
        entry.length  = sg->length;
        m_sglist_vector.push_back(entry);
    }
#else

    m_sglist_initialized = false;
    if (m_hdl->mmap_buffer(&m_mem, m_id, m_size, m_wrapmap) < 0) {
        throw LIBRORC_BUFFER_ERROR_GETMAP_FAILED;
    }

#endif
}



buffer::~buffer()
{
#ifdef PDA
#else
    m_hdl->munmap_buffer(m_mem, m_size, m_wrapmap);
    if (m_free_hdl) {
        delete m_hdl;
    }
#endif
}



bool
buffer::isOvermapped()
{
#ifdef PDA
    void *map_two = NULL;
    if(DMABuffer_getMapTwo(m_buffer, &map_two) != PDA_SUCCESS)
    { throw LIBRORC_BUFFER_ERROR_GETMAPTWO_FAILED; }

    return (map_two!=MAP_FAILED);
#else
    return m_wrapmap;
#endif
}



void
buffer::clear()
{
    memset(m_mem, 0, m_size);
}



int32_t
buffer::deallocate()
{
#ifdef PDA
    if(PciDevice_deleteDMABuffer(m_device, m_buffer) != PDA_SUCCESS)
    { return 1; }

    m_device = NULL;
    m_buffer = NULL;
    m_sglist = NULL;

    m_sglist_vector.erase(m_sglist_vector.begin(), m_sglist_vector.end());

    m_sglist_vector.clear();
    m_mem                          = NULL;
    m_id                           = 0;
    m_numberOfScatterGatherEntries = 0;
    m_size                         = 0;

    return 0;
#else

    m_hdl->munmap_buffer(m_mem, m_size, m_wrapmap);
    m_sglist_vector.clear();
    return m_hdl->deallocate_buffer(m_id);
#endif
}

#ifndef PDA
int
buffer::initializeSglist() {
    ssize_t sgmapsize = m_hdl->get_sglist_size(m_id);
    if( sgmapsize < 0) {
        return -1;
    }

    struct scatter *sglist;
    if( m_hdl->mmap_sglist( (void **)&sglist, m_id, sgmapsize) < 0) {
        return -1;
    }

    uint64_t nsg = sgmapsize / sizeof(struct scatter);
    uint64_t i = 0;
    while (i < nsg) {
        ScatterGatherEntry entry;
        entry.length = sglist[i].length;
        entry.pointer = sglist[i].dma_address;
        // combine with following entries if possible, but make sure length
        // does not exceed 32 bit.
        while( i < (nsg - 1) &&
                (entry.pointer + entry.length
                 == sglist[i+1].dma_address) &&
                (entry.length + sglist[i].length < 0xffffffff) ) {
            entry.length += sglist[i+1].length;
            i++;
        }
        m_sglist_vector.push_back(entry);
        i++;

    }
    m_numberOfScatterGatherEntries = m_sglist_vector.size();
    m_sglist_initialized = true;
    return 0;
}
#endif

std::vector<ScatterGatherEntry>
buffer::sgList()
{
#ifndef PDA
    if(!m_sglist_initialized) {
        initializeSglist();
    }
#endif
    return m_sglist_vector;
}

uint64_t
buffer::getnSGEntries()
{
#ifndef PDA
    if(!m_sglist_initialized) {
        initializeSglist();
    }
#endif
    return m_numberOfScatterGatherEntries;
}

bool
buffer::offsetToPhysAddr
(
    uint64_t offset,
    uint64_t *phys_addr,
    uint64_t *rem_sg_length
)
{
    std::vector<ScatterGatherEntry>::iterator iter, end;
    uint64_t reference_offset = 0;
#ifndef PDA
    if(!m_sglist_initialized) {
        initializeSglist();
    }
#endif
    iter = m_sglist_vector.begin();
    end = m_sglist_vector.end();
    while( iter != end )
    {
        if( offset>=reference_offset && offset<(reference_offset+iter->length) )
        {
            *phys_addr = (uint64_t)iter->pointer + (offset - reference_offset);
            *rem_sg_length = (uint64_t)iter->length - (offset - reference_offset);
            return true;
        }
        else
        { reference_offset += iter->length; }
        ++iter;
    }
    return false;
}


bool
buffer::physAddrToOffset
(
    uint64_t phys_addr,
    uint64_t *offset
)
{
    std::vector<ScatterGatherEntry>::iterator iter, end;
    uint64_t reference_offset = 0;
#ifndef PDA
    if(!m_sglist_initialized) {
        initializeSglist();
    }
#endif
    iter = m_sglist_vector.begin();
    end = m_sglist_vector.end();
    while( iter != end )
    {
        if( phys_addr>=iter->pointer && phys_addr<(iter->pointer+iter->length) )
        {
            *offset = reference_offset + (phys_addr - iter->pointer);
            return true;
        }
        else
        { reference_offset += iter->length; }
        ++iter;
    }
    return false;
}


bool
buffer::composeSglistFromBufferSegment
(
    uint64_t offset,
    uint64_t size,
    std::vector<ScatterGatherEntry> *list
)
{
    uint64_t rem_size = size;
    uint64_t cur_offset = offset;
    uint64_t page_mask = PAGE_SIZE - 1;

    while(rem_size > 0)
    {
        uint64_t phys_addr;
        uint64_t segment_length;

        // get physical address and remaining segment length of current offset
        if( !offsetToPhysAddr(cur_offset, &phys_addr, &segment_length) )
        {
            return false;
        }

        // A PCIe read request may never cross a page boundary:
        // if the physical address is not on a page boundary,
        // go to the next boundary first
        if( phys_addr & page_mask )
        { segment_length = PAGE_SIZE - (phys_addr & page_mask); }

        // reduce segment length to a 32bit value if larger
        if( segment_length>>32 )
        { segment_length = (((uint64_t)1)<<32) - PAGE_SIZE; }

        if( segment_length >= rem_size )
        { segment_length = rem_size; }

        // add this as a new ScatterGatherEntry
        ScatterGatherEntry entry;
        entry.pointer = phys_addr;
        entry.length = segment_length;
        list->push_back(entry);

        // adjust offset and length
        rem_size -= segment_length;
        cur_offset += segment_length;

        // wrap offset if required
        if( cur_offset == getPhysicalSize() )
        { cur_offset = 0; }
    }
    return true;
}
}
