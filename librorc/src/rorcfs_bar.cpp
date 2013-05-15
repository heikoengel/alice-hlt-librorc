/**
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
 * @date 2011-08-16
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
 */

#include "rorcfs_bar.hh"

#include "rorcfs_bar_includes.hh"

#include "rorcfs_device.hh"
#include "rorcfs_dma_channel.hh"
#include <librorc_registers.h>



rorcfs_bar::rorcfs_bar
(
    rorcfs_device *dev,
    int            n
)
{
    m_parent_dev = dev;
    m_number     = n;

    m_pda_pci_device = dev->getPdaPciDevice();

    /** initialize mutex */
    pthread_mutex_init(&m_mtx, NULL);
}



rorcfs_bar::~rorcfs_bar()
{
    pthread_mutex_destroy(&m_mtx);
    /** Further stuff here **/
}



/**
 * rorcfs_bar::init()
 * Initialize and mmap BAR
 * */

int
rorcfs_bar::init()
{
    m_bar = m_parent_dev->getBarMap(m_number);

    if(m_bar == NULL)
    {
        return -1;
    }

    m_size = m_parent_dev->getBarSize(m_number);

    return 0;
}



/**
 * read 1 DW from BAR
 * @param addr address (DW-aligned)
 * @return value read from BAR
 * */

unsigned int
rorcfs_bar::get
(
    unsigned long addr
)
{
    assert( m_bar != NULL );

    unsigned int *bar = (unsigned int *)m_bar;
    unsigned int result;
    if( (addr << 2) < m_size)
    {
        result = bar[addr];
        return result;
    }
    else
    {
        return -1;
    }
}



/**
 * write 1 DW to BAR
 * @param addr DW-aligned address
 * @param data DW data to be written
 * */

void
rorcfs_bar::set
(
    unsigned long addr,
    unsigned int  data
)
{
    assert( m_bar != NULL );
    unsigned int *bar = (unsigned int *)m_bar;
    if( (addr << 2) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        bar[addr] = data;
        msync( (bar + ( (addr << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
        pthread_mutex_unlock(&m_mtx);
    }
}



/**
 * copy a buffer to BAR via memcpy
 * @param addr DW-aligned address
 * @param source source buffer
 * @param num number of bytes to be copied from source to dest
 * */

void
rorcfs_bar::memcpy_bar
(
    unsigned long addr,
    const void   *source,
    size_t        num
)
{
    pthread_mutex_lock(&m_mtx);
    memcpy( (unsigned char*)m_bar + (addr << 2), source, num);
    msync( (m_bar + ( (addr << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
    pthread_mutex_unlock(&m_mtx);
}



unsigned short
rorcfs_bar::get16
(
    unsigned long addr
)
{
    unsigned short *sbar;
    sbar = (unsigned short*)m_bar;
    unsigned short result;
    assert( sbar != NULL );
    if( (addr << 1) < m_size)
    {
        result = sbar[addr];
        return result;
    }
    else
    {
        return 0xffff;
    }
}



void
rorcfs_bar::set16
(
    unsigned long  addr,
    unsigned short data
)
{
    unsigned short *sbar;
    sbar = (unsigned short*)m_bar;

    assert( sbar != NULL );
    if( (addr << 1) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        sbar[addr] = data;
        msync( (sbar + ( (addr << 1) & PAGE_MASK) ),
               PAGE_SIZE, MS_SYNC);
        pthread_mutex_unlock(&m_mtx);
    }
}



int
rorcfs_bar::gettime
(
    struct timeval *tv,
    struct timezone *tz
)
{
    return gettimeofday(tv, tz);
}
