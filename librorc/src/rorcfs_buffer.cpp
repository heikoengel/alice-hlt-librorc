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

#include <stdio.h>
#include <unistd.h>


#include <cstdlib>
#include <cstring>
#include <iostream>

#include <pda.h>

#include "rorcfs_device.hh"
#include "rorcfs_buffer.hh"

using namespace std;



rorcfs_buffer::rorcfs_buffer()
{
    m_id                           = 0;
    m_dmaDirection                 = 0;
    m_mem                          = NULL;
    m_buffer                       = NULL;
    m_numberOfScatterGatherEntries = 0;
}



rorcfs_buffer::~rorcfs_buffer()
{

}



int32_t
rorcfs_buffer::isOvermapped()
{
    void *map_two = NULL;

    if(DMABuffer_getMapTwo(m_buffer, &map_two) != PDA_SUCCESS)
    {
        if(map_two != NULL)
        {
            return 1;
        }
    }

    return 0;
}



uint64_t
rorcfs_buffer::getSize()
{
    return m_size;
}



int32_t
rorcfs_buffer::allocate
(
    rorcfs_device *dev,
    uint64_t       size,
    uint64_t       id,
    int32_t        overmap,
    int32_t        dma_direction
)
{
    m_device =
        dev->getPdaPciDevice();

    cout << "buffer id : " << id << endl;

    //TODO: convert direction specifier

    m_buffer = NULL;
    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    {
	cout << "reallocate : " << size << endl;
        if
        (
            PDA_SUCCESS !=
                PciDevice_allocDMABuffer
                    (m_device, size, PDABUFFER_DIRECTION_BI, &m_buffer)
        )
        {
            cout << "DMA Buffer allocation failed!" << endl;
            return -1;
        }
    }

    /** Wrap map if wanted */
    if(overmap == 1)
    {
        if(PDA_SUCCESS != DMABuffer_wrapMap(m_buffer) )
        {
            cout << "Wrap mapping failed!" << endl;
            return(-1);
        }
    }

    m_dmaDirection = dma_direction;
    m_id           = id;
    m_size         = size;

    return connect(dev, id);
}



int32_t
rorcfs_buffer::deallocate()
{
    return 0;
}



int32_t
rorcfs_buffer::connect
(
    rorcfs_device *dev,
    uint64_t  id
)

{
    /**
     * Check if m_buffer has been set before by allocate(),
     * else lookup buffer by dev and id
     **/
    if ( !m_buffer )
    {
        if ( PciDevice_getDMABuffer(dev->getPdaPciDevice(), id, &m_buffer)!=PDA_SUCCESS )
        {
            cout << "Buffer lookup failed!" << endl;
            return -1;
        }
    }

    if( DMABuffer_getMap(m_buffer, (void**)(&m_mem) )!=PDA_SUCCESS )
    {
        cout << "Mapping failed!" << endl;
        return -1;
    }

    DMABuffer_SGNode *sglist;
    if(DMABuffer_getSGList(m_buffer, &sglist)!=PDA_SUCCESS)
    {
        cout << "SG list lookup failed!" << endl;
        return -1;
    }

    for(DMABuffer_SGNode *sg=sglist; sg!=NULL; sg=sg->next)
    {
        m_numberOfScatterGatherEntries++;
    }

    return 0;
}
