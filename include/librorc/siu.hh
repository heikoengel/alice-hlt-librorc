/**
 * @file siu.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-15
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
 * */

#ifndef LIBRORC_SIU_H
#define LIBRORC_SIU_H

#include <librorc/ddl.hh>
#include <librorc/defines.hh>

namespace LIBRARY_NAME
{
    class link;
    class ddl;

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
            link *m_link;
    };
}
#endif
