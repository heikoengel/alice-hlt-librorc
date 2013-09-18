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

#include <librorc/registers.h>
#include <librorc/link.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>

#include <pda.h>

using namespace std;

namespace librorc
{
    link::link
    (
        bar      *bar,
        uint32_t  link_number
    )
    {
        m_bar         = bar;
        m_base        = (link_number + 1) * RORC_CHANNEL_OFFSET;
        m_link_number = link_number;
    };



    void
    link::setGTX
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32( m_base+(1<<RORC_DMA_CMP_SEL)+addr, data);
    }



    uint32_t
    link::getGTX(uint32_t addr)
    {
        return m_bar->get32(m_base+(1<<RORC_DMA_CMP_SEL)+addr);
    }



    void
    link::setPacketizer
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32((m_base + addr), data);
    }



    uint32_t
    link::packetizer
    (
        uint32_t addr
    )
    {
        return m_bar->get32(m_base+addr);
    }
}
