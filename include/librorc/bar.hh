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
 *
 * @section DESCRIPTION
 *
 * The rorc_bar class represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 */

#ifndef LIBRORC_BAR_H
#define LIBRORC_BAR_H

#include <librorc/include_ext.hh>
#include <librorc/include_int.hh>



/**
 * @class rorc_bar
 * @brief Represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 *
 * Create a new crorc_bar object after initializing your
 * librorc::device instance. <br>Once your rorc_bar instance is
 * initialized (with init()) you can use get() and set() to
 * read from and/or write to the device.
 */

namespace librorc
{

    class rorc_bar : public bar
    {
        public:

            rorc_bar
            (
                device  *dev,
                int32_t  n
            );

            ~rorc_bar();

            void
            memcopy
            (
                librorc_bar_address  target,
                const void          *source,
                size_t               num
            );

            void
            memcopy
            (
                void                *target,
                librorc_bar_address  source,
                size_t               num
            );

            uint32_t get( librorc_bar_address address );

            uint16_t get16( librorc_bar_address address );

            void
            set
            (
                librorc_bar_address address,
                uint32_t data
            );

            void
            set16
            (
                librorc_bar_address address,
                uint16_t data
            );

            int32_t
            gettime
            (
                struct timeval *tv,
                struct timezone *tz
            );

            size_t getSize();

    };

}
#endif /** LIBRORC_BAR_H */
