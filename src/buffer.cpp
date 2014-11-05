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

#include <librorc/buffer.hh>
#include <librorc/device.hh>

#include <pda.h>

namespace LIBRARY_NAME
{



buffer::buffer
(
    device   *dev,
    uint64_t  size,
    uint64_t  id,
    int32_t   overmap
)
{
    m_id                = id;
    m_device            = dev->getPdaPciDevice();

    bool newAllocNeeded = false;
    m_buffer            = NULL;

    if( size == 0 )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR, "Requested buffer of size 0\n");
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    { newAllocNeeded = true; }
    else
    {
        size_t length = 0;
        if(DMABuffer_getLength(m_buffer, &length) != PDA_SUCCESS)
        {
            std::cout << "Getting buffer length from PDA totally failed!" << std::endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }

        if(size != length)
        {
            if(PciDevice_deleteDMABuffer(m_device, m_buffer) != PDA_SUCCESS)
            {
                std::cout << "PDA buffer freeing totally failed!" << std::endl;
                throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
            }
            newAllocNeeded = true;
        }
    }

    if(newAllocNeeded)
    {
        if
        (
            PDA_SUCCESS !=
                PciDevice_allocDMABuffer
                    (m_device, id, size, PDABUFFER_DIRECTION_BI, &m_buffer)
        )
        {
            std::cout << "DMA Buffer allocation failed!" << std::endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }
    }

    if(overmap == 1)
    {
        if(PDA_SUCCESS != DMABuffer_wrapMap(m_buffer) )
        {
            std::cout << "Wrap mapping failed!" << std::endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }
    }

    connect();
}



buffer::buffer
(
    device   *dev,
    uint64_t  id,
    int32_t   overmap
)
{
    m_id     = id;
    m_device = dev->getPdaPciDevice();

    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    { throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED; }

    if(overmap == 1)
    {
        if(PDA_SUCCESS != DMABuffer_wrapMap(m_buffer) )
        {
            std::cout << "Wrap mapping failed!" << std::endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }
    }
    connect();
}



void
buffer::connect()

{
    m_size = 0;
    if( DMABuffer_getLength( m_buffer, &m_size) != PDA_SUCCESS )
    {
        std::cout << "Failed to get buffer size!" << std::endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    m_mem = NULL;
    if( DMABuffer_getMap(m_buffer, (void**)(&m_mem) )!=PDA_SUCCESS )
    {
        std::cout << "Mapping failed!" << std::endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    m_sglist = NULL;
    if(DMABuffer_getSGList(m_buffer, &m_sglist)!=PDA_SUCCESS)
    {
        std::cout << "SG list lookup failed!" << std::endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

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
}



buffer::~buffer()
{

}



int32_t
buffer::isOvermapped()
{
    void *map_two = NULL;

    if(DMABuffer_getMapTwo(m_buffer, &map_two) == PDA_SUCCESS)
    {
        if(map_two != MAP_FAILED)
        { return 1; }
        else
        {return 0;}
    }
    else
    { return -1; }
}



void
buffer::clear()
{
    memset(m_mem, 0, m_size);
}



int32_t
buffer::deallocate()
{
    if(PciDevice_deleteDMABuffer(m_device, m_buffer) != PDA_SUCCESS)
    { return 1; }

    m_device = NULL;
    m_buffer = NULL;
    m_sglist = NULL;

    m_sglist_vector.erase(m_sglist_vector.begin(), m_sglist_vector.end());

    m_mem                          = NULL;
    m_id                           = 0;
    m_numberOfScatterGatherEntries = 0;
    m_size                         = 0;

    return 0;
}


bool
buffer::offsetToPhysAddr
(
    uint64_t offset,
    uint64_t *phys_addr,
    uint64_t *rem_sg_length
)
{
    std::vector<ScatterGatherEntry>::iterator iter;
    uint64_t reference_offset = 0;
    for( iter=m_sglist_vector.begin(); iter!=m_sglist_vector.end(); iter++ )
    {
        if( offset>=reference_offset && offset<(reference_offset+iter->length) )
        {
            *phys_addr = (uint64_t)iter->pointer + (offset - reference_offset);
            *rem_sg_length = (uint64_t)iter->length - (offset - reference_offset);
            return true;
        }
        else
        { reference_offset += iter->length; }
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
    std::vector<ScatterGatherEntry>::iterator iter;
    uint64_t reference_offset = 0;
    for( iter=m_sglist_vector.begin(); iter!=m_sglist_vector.end(); iter++ )
    {
        if( phys_addr>=iter->pointer && phys_addr<(iter->pointer+iter->length) )
        {
            *offset = reference_offset + (phys_addr - iter->pointer);
            return true;
        }
        else
        { reference_offset += iter->length; }
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
            DEBUG_PRINTF( PDADEBUG_ERROR, "Failed to convert offset %llx "
                    " to pysical address\n", cur_offset);
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
