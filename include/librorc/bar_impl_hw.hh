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
 *
 * @section DESCRIPTION
 *
 * The bar_impl_hw class represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 */

#ifndef LIBRORC_BAR_IMPL_HW_H
#define LIBRORC_BAR_IMPL_HW_H

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>
#include <librorc/bar.hh>


namespace LIBRARY_NAME
{
    class device;

    /**
     * @brief Represents a Base Address Register (BAR) file
     * mapping of the RORCs PCIe address space
     *
     * Create a new crorc_bar object after initializing your
     * librorc::device instance. <br>Once your bar_impl_hw instance is
     * initialized (with init()) you can use get() and set() to
     * read from and/or write to the device.
     */
    class bar_impl_hw
    {
        public:
            bar_impl_hw
            (
                device  *dev,
                int32_t  n
            );

            ~bar_impl_hw();

            void
            memcopy
            (
                bar_address  target,
                const void          *source,
                size_t               num
            );

            void
            memcopy
            (
                void                *target,
                bar_address  source,
                size_t               num
            );

            uint32_t get32( bar_address address );
            uint16_t get16( bar_address address );

            void
            set32
            (
                bar_address address,
                uint32_t data
            );

            void
            set16
            (
                bar_address address,
                uint16_t data
            );

            int32_t
            gettime
            (
                struct timeval *tv,
                struct timezone *tz
            );

            size_t size();

            void
            simSetPacketSize
            (
                uint32_t packet_size
            );

        protected:
            Bar *m_pda_bar;
  device *m_parent_dev;
  PciDevice *m_pda_pci_device;
  pthread_mutex_t m_mtx;
  int32_t m_number;
  uint8_t *m_bar;
  size_t m_size;
    };

}
#endif /** LIBRORC_BAR_IMPL_HW_H */
