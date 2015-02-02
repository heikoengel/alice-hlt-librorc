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

#ifndef LIBRORC_SIU_H
#define LIBRORC_SIU_H

#include <librorc/ddl.hh>
#include <librorc/defines.hh>

namespace LIBRARY_NAME
{
    class link;

    /**
     * @brief Source Interface Unit (SIU) control class
     *
     * The SIU is the data sink for HLT_OUT.
     **/
    class siu : public ddl
    {
        public:
            siu( link *link );
            ~siu();

            /**
             * check if connected DIU has opened the link / sent
             * RDYRX and SIU is ready to send.
             * @return true when open, false when closed.
             **/
            bool
            linkOpen();

            /**
             * get DDL deadtime
             * @return deadtime in clock cycles. For HLT_OUT this is the
             * number of cycles where DMA data is available but SIU is busy
             * -> should increase.
             **/
            uint32_t
            getDdlDeadtime();

            /**
             * clear DDL deadtime counter */
            void
            clearDdlDeadtime();

            /**
             * get num,ber of events transmitted via SIU
             * @return number of events
             **/
            uint32_t
            getEventcount();

            /**
             * clear SIU event count
             **/
            void
            clearEventcount();

            /**
             * get last FrontEnd Command Word sent to SIU
             * @return last FrontEnd Command Word
             **/
            uint32_t
            lastFrontEndCommandWord();

            /**
             * clear last FrontEnd Command Word. This registers
             * holds 0xffffffff after clearing.
             **/
            void
            clearLastFrontEndCommandWord();

            /**
             * get state of SIU Interface FIFO. If this FIFO is permanently
             * empty there is no data from DMA or PG available.
             * @return true when empty, false when not empty
             **/
            bool
            isInterfaceFifoEmpty();

            /**
             * get state of SIU Interface FIFO. If this FIFO is permanently
             * full the data source (DMA or PG) delivers more data than the
             * SIU can transmit. Also check linkFull() in this case.
             * @return true when full, false when not full
             **/
            bool
            isInterfaceFifoFull();

            /**
             * get state of data source. If source is permanently empty there
             * is no data coming via DMA.
             * @return true when empty, false when not empty.
             **/
            bool
            isSourceEmpty();

        protected:
    };
}
#endif
