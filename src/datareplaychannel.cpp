/**
 * @file datareplaychannel.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-04-16
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

#include<librorc/datareplaychannel.hh>

namespace LIBRARY_NAME
{
    datareplaychannel::datareplaychannel( link *link )
    {
        m_link = link;
    }

    datareplaychannel::~datareplaychannel()
    {
        m_link = NULL;
    }

    bool
    datareplaychannel::isDone()
    {
        uint32_t chsts = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_STS);
        return ((chsts & (1<<0)) != 0);
    }

    bool
    datareplaychannel::isWaiting()
    {
        uint32_t chsts = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_STS);
        return ((chsts & (1<<1)) != 0);
    }

    bool
    datareplaychannel::isOneshotEnabled()
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<2)) != 0);
    }

    bool
    datareplaychannel::isContinuousEnabled()
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<1)) != 0);
    }

    bool
    datareplaychannel::isEnabled()
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<0)) != 0);
    }

    bool
    datareplaychannel::isInReset()
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<31)) != 0);
    }

    void
    datareplaychannel::setEnable
    (
        uint32_t enable
    )
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<0);
        chctrl |= ((enable&1)<<0);
        m_link->setDDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    void
    datareplaychannel::setReset
    (
        uint32_t reset
    )
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<31);
        chctrl |= ((reset&1)<<31);
        m_link->setDDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    void
    datareplaychannel::setModeOneshot
    (
        uint32_t oneshot
    )
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<2);
        chctrl |= ((oneshot&1)<<2);
        m_link->setDDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    void
    datareplaychannel::setModeContinuous
    (
        uint32_t continuous
    )
    {
        uint32_t chctrl = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<1);
        chctrl |= ((continuous&1)<<1);
        m_link->setDDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    uint32_t
    datareplaychannel::nextAddress()
    {
        return (m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_STS) &
                0x7ffffff8);
    }

    uint32_t
    datareplaychannel::startAddress()
    {
        return (m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL) &
                0x7ffffff8);
    }

    void
    datareplaychannel::setStartAddress
    (
        uint32_t start_address
    )
    {
        uint32_t ch_cfg = m_link->DDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        ch_cfg &= ~(0x7ffffff8); // clear [30:3]
        ch_cfg |= (start_address & 0x7ffffff8); //set start_address[30:3]
        m_link->setDDL(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, ch_cfg);
    }

}
