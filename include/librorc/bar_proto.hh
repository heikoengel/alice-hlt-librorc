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

#ifndef LIBRORC_BAR_PROTO_H
#define LIBRORC_BAR_PROTO_H

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>

#define LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED  1

typedef uint64_t librorc_bar_address;


namespace LIBRARY_NAME
{
class device;

    class bar
    {
    public:

    virtual ~bar() {}

    /**
     * copy buffer from host to device and vice versa
     * @param target address
     * @param source address
     * @param num number of bytes to be copied to destination
     * */
    virtual
    void
    memcopy
    (
        librorc_bar_address  target,
        const void          *source,
        size_t               num
    ) = 0;

    virtual
    void
    memcopy
    (
        void                *target,
        librorc_bar_address  source,
        size_t               num
    ) = 0;

    /**
     * read DWORD from BAR address
     * @param addr (unsigned int) aligned address within the
     *              BAR to read from.
     * @return data read from BAR[addr]
     **/
    virtual uint32_t get32(librorc_bar_address address ) = 0;

    /**
     * read WORD from BAR address
     * @param addr within the BAR to read from.
     * @return data read from BAR[addr]
     **/
    virtual uint16_t get16(librorc_bar_address address ) = 0;

    /**
     * write DWORD to BAR address
     * @param addr (unsigned int) aligned address within the
     *              BAR to write to
     * @param data (unsigned int) data word to be written.
     **/
    virtual
    void
    set32
    (
        librorc_bar_address address,
        uint32_t data
    ) = 0;

    /**
     * write WORD to BAR address
     * @param addr within the BAR to write to
     * @param data (unsigned int) data word to be written.
     **/
    virtual
    void
    set16
    (
        librorc_bar_address address,
        uint16_t data
    ) = 0;

    /**
     * get current time of day
     * @param tv pointer to struct timeval
     * @param tz pointer to struct timezone
     * @return return value from gettimeof day or zero for FLI simulation
     **/
    virtual
    int32_t
    gettime
    (
        struct timeval *tv,
        struct timezone *tz
    ) = 0;

    /**
     * get size of mapped BAR. This value is only valid after init()
     * @return size of mapped BAR in bytes
     **/
    virtual size_t size() = 0;

    virtual
    void
    simSetPacketSize
    (
        uint32_t packet_size
    ){};

    protected:
        device          *m_parent_dev;
        PciDevice       *m_pda_pci_device;
        pthread_mutex_t  m_mtx;
        int32_t          m_number;
        uint8_t         *m_bar;
        size_t           m_size;

    };

}
#endif /** LIBRORC_BAR_PROTO_H */
