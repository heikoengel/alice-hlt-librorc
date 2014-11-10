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

#ifndef LIBRORC_DDL_H
#define LIBRORC_DDL_H

#include <librorc/defines.hh>
#include<librorc/link.hh>
#include<librorc/registers.h>

namespace LIBRARY_NAME
{
    class link;

    /**
     * @brief wrapper class for common DIU/SIU methods
     **/
    class ddl
    {
        public:
            ddl( link *link );
            virtual ~ddl();

            /**
             * check if DDL link is full
             * @return true when full, false when not full
             **/
            bool linkFull();

            /**
             * set DDL module reset
             * @param value reset value
             **/
            void setReset
            (
                uint32_t value
            );

            /**
             * get DDL module reset
             * @return reset value
             **/
            uint32_t
            getReset();

            /**
             * set DDL interface enable 
             * @param value enable value
             **/
            void setEnable
            (
                uint32_t value
            );

            /**
             * get DDL interface enable
             * @return enable value
             **/
            uint32_t
            getEnable();

            /**
             * get DMA deadtime
             * @return number of cycles when DMA input FIFO was full while
             * DDL channel was active
             **/
            uint32_t
            getDmaDeadtime();

            /**
             * clear DMA deadtime counter */
            void
            clearDmaDeadtime();
            
            /**
             * get num,ber of events received (DIU) or transmitted (SIU)
             * @return number of events
             **/
            uint32_t
            getEventcount();

            /**
             * clear DIU/SIU event count
             **/
            void
            clearEventcount();

        protected:
            link *m_link;
    };
}
#endif
