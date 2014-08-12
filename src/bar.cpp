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

#include <librorc/registers.h>
#include <librorc/device.hh>
#include <librorc/bar.hh>

using namespace std;

namespace LIBRARY_NAME
{

rorc_bar::rorc_bar
(
    device  *dev,
    int32_t  n
)
{
    m_parent_dev     = dev;
    m_number         = n;
    m_pda_pci_device = dev->getPdaPciDevice();
    m_bar            = m_parent_dev->getBarMap(m_number);

    if(m_bar == NULL)
    { throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED; }

    m_size = m_parent_dev->getBarSize(m_number);

    m_pda_bar = NULL;
    if( PDA_SUCCESS != PciDevice_getBar(m_pda_pci_device, &m_pda_bar, m_number) )
    { throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED; }

    pthread_mutex_init(&m_mtx, NULL);
}



rorc_bar::~rorc_bar()
{
    pthread_mutex_destroy(&m_mtx);
}



void
rorc_bar::memcopy
(
    librorc_bar_address  target,
    const void          *source,
    size_t               num
)
{
    pthread_mutex_lock(&m_mtx);
    if( PDA_SUCCESS != Bar_memcpyToBar32(m_pda_bar, (target << 2), source, num) )
    { cout << "bar copy failed!" << endl; }
    msync( (uint8_t*)m_bar + ((target << 2) & PAGE_MASK) , PAGE_SIZE, MS_SYNC);
    pthread_mutex_unlock(&m_mtx);
}



void
rorc_bar::memcopy
(
    void                *target,
    librorc_bar_address  source,
    size_t               num
)
{
    assert(false);
    pthread_mutex_lock(&m_mtx);
    if( PDA_SUCCESS != Bar_memcpyFromBar32(m_pda_bar, target, source, num) )
    { cout << "bar copy failed!" << endl; }
    pthread_mutex_unlock(&m_mtx);
}



uint32_t
rorc_bar::get32(librorc_bar_address address )
{
//    uint32_t *bar = (uint32_t *)m_bar;
//    uint32_t result;
//    if( (address << 2) < m_size)
//    {
//        result = bar[address];
//        return result;
//    }
//    else
//    { return -1; }

    return( Bar_get32(m_pda_bar, (address*4) ) );
}



uint16_t
rorc_bar::get16(librorc_bar_address address )
{
//    uint16_t *sbar;
//    sbar = (uint16_t *)m_bar;
//    assert( sbar != NULL );
//
//    uint64_t result;
//    if( (address << 1) < m_size)
//    {
//        result = sbar[address];
//        return result;
//    }
//    else
//    { return 0xffff; }

    return( Bar_get16(m_pda_bar, (address*2) ) );
}



void
rorc_bar::set32
(
    librorc_bar_address address,
    uint32_t data
)
{
    uint32_t *bar = (uint32_t *)m_bar;
    assert( m_bar != NULL );
    if( (address << 2) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        //bar[address] = data;
        Bar_put32(m_pda_bar, data, (address*4) );
        msync( (bar + ( (address << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
        pthread_mutex_unlock(&m_mtx);
    }

}



void
rorc_bar::set16
(
    librorc_bar_address address,
    uint16_t data
)
{
    uint16_t *sbar;
    sbar = (uint16_t *)m_bar;

    assert( sbar != NULL );
    if( (address << 1) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        //sbar[address] = data;
        Bar_put32(m_pda_bar, data, (address*2) );
        msync( (sbar + ( (address << 1) & PAGE_MASK) ),
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
rorc_bar::size()
{
    return m_size;
}

}
