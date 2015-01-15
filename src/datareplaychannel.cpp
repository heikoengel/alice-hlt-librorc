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

#include <librorc/datareplaychannel.hh>
#include <librorc/link.hh>

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
        uint32_t chsts = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_STS);
        return ((chsts & (1<<0)) != 0);
    }

    bool
    datareplaychannel::isWaiting()
    {
        uint32_t chsts = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_STS);
        return ((chsts & (1<<1)) != 0);
    }

    bool
    datareplaychannel::isOneshotEnabled()
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<2)) != 0);
    }

    bool
    datareplaychannel::isContinuousEnabled()
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<1)) != 0);
    }

    bool
    datareplaychannel::isEnabled()
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<0)) != 0);
    }

    bool
    datareplaychannel::isInReset()
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        return ((chctrl & (1<<31)) != 0);
    }

    void
    datareplaychannel::setEnable
    (
        uint32_t enable
    )
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<0);
        chctrl |= ((enable&1)<<0);
        m_link->setDdlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    void
    datareplaychannel::setReset
    (
        uint32_t reset
    )
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<31);
        chctrl |= ((reset&1)<<31);
        m_link->setDdlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    void
    datareplaychannel::setModeOneshot
    (
        uint32_t oneshot
    )
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<2);
        chctrl |= ((oneshot&1)<<2);
        m_link->setDdlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    void
    datareplaychannel::setModeContinuous
    (
        uint32_t continuous
    )
    {
        uint32_t chctrl = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        chctrl &= ~(1<<1);
        chctrl |= ((continuous&1)<<1);
        m_link->setDdlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, chctrl);
    }

    uint32_t
    datareplaychannel::nextAddress()
    {
        return (m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_STS) &
                0x7ffffff8);
    }

    uint32_t
    datareplaychannel::startAddress()
    {
        return (m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL) &
                0x7ffffff8);
    }

    void
    datareplaychannel::setStartAddress
    (
        uint32_t start_address
    )
    {
        uint32_t ch_cfg = m_link->ddlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        ch_cfg &= ~(0x7ffffff8); // clear [30:3]
        ch_cfg |= (start_address & 0x7ffffff8); //set start_address[30:3]
        m_link->setDdlReg(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, ch_cfg);
    }

}
