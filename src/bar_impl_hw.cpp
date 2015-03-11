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
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#include <librorc/device.hh>
#include <librorc/error.hh>
#include <librorc/bar_impl_hw.hh>

namespace LIBRARY_NAME
{

bar_impl_hw::bar_impl_hw
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

//    m_pda_bar = NULL;
//    if( PDA_SUCCESS != PciDevice_getBar(m_pda_pci_device, &m_pda_bar, m_number) )
//    { throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED; }

    pthread_mutex_init(&m_mtx, NULL);
}



bar_impl_hw::~bar_impl_hw()
{
    pthread_mutex_destroy(&m_mtx);
}


__attribute__((optimize("no-tree-vectorize")))
__attribute__((__target__("no-sse")))
void
bar_impl_hw::memcopy
(
    bar_address  target,
    const void          *source,
    size_t               num
)
{
    pthread_mutex_lock(&m_mtx);
//    if( PDA_SUCCESS != Bar_memcpyToBar32(m_pda_bar, (target << 2), source, num) )
//    { cout << "bar copy failed!" << endl; }
    memcpy( (uint8_t*)m_bar + (target << 2), source, num);
    msync( (uint8_t*)m_bar + ((target << 2) & PAGE_MASK) , PAGE_SIZE, MS_SYNC);
    pthread_mutex_unlock(&m_mtx);
}



__attribute__((optimize("no-tree-vectorize")))
__attribute__((__target__("no-sse")))
void
bar_impl_hw::memcopy
(
    void                *target,
    bar_address  source,
    size_t               num
)
{
    assert(false);
    pthread_mutex_lock(&m_mtx);
//    if( PDA_SUCCESS != Bar_memcpyFromBar32(m_pda_bar, target, source, num) )
//    { cout << "bar copy failed!" << endl; }
    memcpy( target, (const void*)(m_bar + (source << 2)), num);
    pthread_mutex_unlock(&m_mtx);
}



__attribute__((optimize("no-tree-vectorize")))
__attribute__((__target__("no-sse")))
uint32_t
bar_impl_hw::get32(bar_address address )
{
    uint32_t *bar = (uint32_t *)m_bar;
    uint32_t result;
    if( (address << 2) < m_size)
    {
        result = bar[address];
        return result;
    }
    else
    { return -1; }
}



__attribute__((optimize("no-tree-vectorize")))
__attribute__((__target__("no-sse")))
uint16_t
bar_impl_hw::get16(bar_address address )
{
    uint16_t *sbar;
    sbar = (uint16_t *)m_bar;
    assert( sbar != NULL );

    uint64_t result;
    if( (address << 1) < m_size)
    {
        result = sbar[address];
        return result;
    }
    else
    { return 0xffff; }
}



__attribute__((optimize("no-tree-vectorize")))
__attribute__((__target__("no-sse")))
void
bar_impl_hw::set32
(
    bar_address address,
    uint32_t data
)
{
    uint32_t *bar = (uint32_t *)m_bar;
    assert( m_bar != NULL );
    if( (address << 2) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        bar[address] = data;
        msync( (bar + ( (address << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
        pthread_mutex_unlock(&m_mtx);
    }

}



__attribute__((optimize("no-tree-vectorize")))
__attribute__((__target__("no-sse")))
void
bar_impl_hw::set16
(
    bar_address address,
    uint16_t data
)
{
    uint16_t *sbar;
    sbar = (uint16_t *)m_bar;

    assert( sbar != NULL );
    if( (address << 1) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        sbar[address] = data;
        msync( (sbar + ( (address << 1) & PAGE_MASK) ),
               PAGE_SIZE, MS_SYNC);
        pthread_mutex_unlock(&m_mtx);
    }
}



int32_t
bar_impl_hw::gettime
(
    struct timeval *tv,
    struct timezone *tz
)
{
    return gettimeofday(tv, tz);
}

void
bar_impl_hw::simSetPacketSize
(
    uint32_t packet_size
){}

size_t
bar_impl_hw::size()
{
    return m_size;
}

}
