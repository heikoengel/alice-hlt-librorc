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
 * @section DESCRIPTION
 *
 */

#include <librorc/buffer.hh>
#include <librorc/device.hh>

#include <pda.h>

using namespace std;

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
            cout << "Getting buffer length from PDA totally failed!" << endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }

        if(size != length)
        {
            if(DMABuffer_free(m_buffer, PDA_DELETE) != PDA_SUCCESS)
            {
                cout << "PDA buffer freeing totally failed!" << endl;
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
            cout << "DMA Buffer allocation failed!" << endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }
    }

    if(overmap == 1)
    {
        if(PDA_SUCCESS != DMABuffer_wrapMap(m_buffer) )
        {
            cout << "Wrap mapping failed!" << endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }
    }

    connect(dev, id);
}



buffer::buffer
(
    device   *dev,
    uint64_t  id,
    int32_t   overmap
)
{
    m_device            = dev->getPdaPciDevice();
    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    { throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED; }

    if(overmap == 1)
    {
        if(PDA_SUCCESS != DMABuffer_wrapMap(m_buffer) )
        {
            cout << "Wrap mapping failed!" << endl;
            throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
        }
    }
    connect(dev, id);
}



void
buffer::connect
(
    device   *dev,
    uint64_t  id
)

{
    m_id                           = id;
    m_device                       = dev->getPdaPciDevice();

    if( PciDevice_getDMABuffer(dev->getPdaPciDevice(), id, &m_buffer)!=PDA_SUCCESS )
    {
        cout << "Buffer lookup failed!" << endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    m_size = 0;
    if( DMABuffer_getLength( m_buffer, &m_size) != PDA_SUCCESS )
    {
        cout << "Failed to get buffer size!" << endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    m_mem = NULL;
    if( DMABuffer_getMap(m_buffer, (void**)(&m_mem) )!=PDA_SUCCESS )
    {
        cout << "Mapping failed!" << endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    m_sglist = NULL;
    if(DMABuffer_getSGList(m_buffer, &m_sglist)!=PDA_SUCCESS)
    {
        cout << "SG list lookup failed!" << endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    m_numberOfScatterGatherEntries = 0;
    for(DMABuffer_SGNode *sg=m_sglist; sg!=NULL; sg=sg->next)
    { m_numberOfScatterGatherEntries++; }

    for(DMABuffer_SGNode *sg=m_sglist; sg!=NULL; sg=sg->next)
    {
        librorc_sg_entry entry;
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
    if(DMABuffer_free(m_buffer, PDA_DELETE) != PDA_SUCCESS)
    { return 1; }

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
    std::vector<librorc_sg_entry>::iterator iter;
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
    std::vector<librorc_sg_entry>::iterator iter;
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
    std::vector<librorc_sg_entry> *list
)
{
    uint64_t rem_size = size;
    uint64_t cur_offset = offset;
    while(rem_size > 0)
    {
        uint64_t phys_addr;
        uint64_t segment_length;

        // get physical address and remaining length of current sg entry
        if( !offsetToPhysAddr(cur_offset, &phys_addr, &segment_length) )
        {
            DEBUG_PRINTF( PDADEBUG_ERROR, "Failed to convert offset %llx "
                    " to pysical address\n", cur_offset);
            return false;
        }

        // reduce segment length to a 32bit value if larger
        if( segment_length>>32 )
        { segment_length = (((uint64_t)1)<<32) - PAGE_SIZE; }

        if( segment_length >= rem_size )
        { segment_length = rem_size; }

        // add this as a new librorc_sg_entry
        librorc_sg_entry entry;
        entry.pointer = phys_addr;
        entry.length = segment_length;
        list->push_back(entry);

        // adjust offset and length
        rem_size -= segment_length;
        cur_offset += segment_length;
    }
    return true;
}

}
