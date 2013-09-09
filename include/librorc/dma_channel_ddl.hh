/**
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2013-08-29
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

#ifndef LIBRORC_DMA_CHANNEL_DDL_H
#define LIBRORC_DMA_CHANNEL_DDL_H

#include <librorc/dma_channel.hh>

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>

namespace LIBRARY_NAME
{

class bar;
class buffer;
class device;

    class dma_channel_ddl : public dma_channel
    {
        public:
             dma_channel_ddl
             (
                uint32_t  channel_number,
                device   *dev,
                bar      *bar
             );

             dma_channel_ddl
             (
                uint32_t  channel_number,
                uint32_t  pcie_packet_size,
                device   *dev,
                bar      *bar,
                buffer   *eventBuffer,
                buffer   *reportBuffer
             );

            ~dma_channel_ddl();


        protected:
            void configureDDL();
            void closeDDL();

    };

}

#endif /** LIBRORC_DMA_CHANNEL_DDL_H */