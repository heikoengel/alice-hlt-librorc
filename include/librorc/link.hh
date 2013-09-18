/**
 * @file
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>, Heiko Engel <hengel@cern.ch>
 * @date 2011-09-18
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
 */

#ifndef LIBRORC_LINK_H
#define LIBRORC_LINK_H

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>

namespace librorc
{
class bar;
class device;

    class link
    {
        public:
            link(){};

            link
            (
                bar      *bar,
                uint32_t  link_number
            );

            virtual ~link(){};


            /**
             * set DW in GTX Domain
             * @param addr address in GTX component
             * @param data data to be writtem
             **/
            void
            setGTX
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from GTX Domain
             * @param addr address in GTX component
             * @return data read from GTX
             **/
            unsigned int getGTX(uint32_t addr);

        protected:
            bar      *m_bar;
            uint32_t  m_base;
            uint32_t  m_link_number;

    };

}

#endif /** LIBRORC_LINK_H */
