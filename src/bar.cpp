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

#include <librorc/bar.hh>
#include <librorc/registers.h>

namespace librorc
{

rorc_bar::rorc_bar
(
    device  *dev,
    int32_t  n
)
{
    m_parent_dev = dev;
    m_number     = n;

    m_pda_pci_device = dev->getPdaPciDevice();

    m_bar = m_parent_dev->getBarMap(m_number);

    if(m_bar == NULL)
    {
        throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED;
    }

    m_size = m_parent_dev->getBarSize(m_number);

    /** initialize mutex */
    pthread_mutex_init(&m_mtx, NULL);
}



rorc_bar::~rorc_bar()
{
    pthread_mutex_destroy(&m_mtx);
    /** Further stuff here **/
}



///**
// * Initialize and mmap BAR
// * */
//
//int32_t
//rorc_bar::init()
//{
//    m_bar = m_parent_dev->getBarMap(m_number);
//
//    if(m_bar == NULL)
//    {
//        return -1;
//    }
//
//    m_size = m_parent_dev->getBarSize(m_number);
//
//    return 0;
//
//}



/**
 * read 1 DW from BAR
 * @param addr address (DW-aligned)
 * @return value read from BAR
 * */

uint32_t
rorc_bar::get
(
    uint64_t addr
)
{
    assert( m_bar != NULL );

    uint32_t *bar = (uint32_t *)m_bar;
    uint32_t result;
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
rorc_bar::set
(
    uint64_t addr,
    uint32_t data
)
{
    uint32_t *bar = (uint32_t *)m_bar;
    assert( m_bar != NULL );
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
rorc_bar::memcpy_bar
(
    uint64_t    addr,
    const void *source,
    size_t      num
)
{
    pthread_mutex_lock(&m_mtx);
    memcpy( (unsigned char*)m_bar + (addr << 2), source, num);
    msync( (m_bar + ( (addr << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
    pthread_mutex_unlock(&m_mtx);
}



unsigned short
rorc_bar::get16
(
    uint64_t addr
)
{
    uint16_t *sbar;
    sbar = (uint16_t *)m_bar;
    assert( sbar != NULL );

    uint64_t result;
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
rorc_bar::set16
(
    uint64_t addr,
    uint16_t data
)
{
    uint16_t *sbar;
    sbar = (uint16_t *)m_bar;

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



int32_t
rorc_bar::gettime
(
    struct timeval *tv,
    struct timezone *tz
)
{
    return gettimeofday(tv, tz);
}



size_t
rorc_bar::getSize()
{
    return m_size;
}

}
