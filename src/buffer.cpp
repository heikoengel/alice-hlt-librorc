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
    int32_t   overmap,
    int32_t   dma_direction
)
{
    m_dmaDirection      = dma_direction;
    m_device            = dev->getPdaPciDevice();
    bool newAllocNeeded = false;

    m_buffer = NULL;
    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    { newAllocNeeded = true; }

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
    uint64_t  id
)
{
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

    if ( PciDevice_getDMABuffer(dev->getPdaPciDevice(), id, &m_buffer)!=PDA_SUCCESS )
    {
        cout << "Buffer lookup failed!" << endl;
        throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED;
    }

    m_size = 0;
    if ( DMABuffer_getLength( m_buffer, &m_size) != PDA_SUCCESS )
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



buffer::~buffer(){}



int32_t
buffer::isOvermapped()
{
    void *map_two = NULL;

    if(DMABuffer_getMapTwo(m_buffer, &map_two) != PDA_SUCCESS)
    { if(map_two != NULL){ return 1; } }

    return 0;
}



void
buffer::clear()
{
    memset(m_mem, 0, m_size);
}



int32_t
buffer::deallocate()
{
    return 0;
}



}
