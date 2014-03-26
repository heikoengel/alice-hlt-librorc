/**
 * @file ddl.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-13
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
            ~ddl();


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
             * enable DIU/SIU Interface
             **/
            void
            enableInterface();


            /**
             * disable DIU/SIU Interface
             **/
            void
            disableInterface();


            /**
             * get DDL deadtime
             * @return deadtime in clock cycles. For HLT_IN this is the
             * number of cycles where XOFF was active -> should not
             * increase. For HLT_OUT this is the number of cycles where
             * DMA data is available but SIU is busy -> should increase.
             **/
            uint32_t
            getDeadtime();

            /**
             * clear DDL deadtime counter */
            void
            clearDeadtime();
            
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
