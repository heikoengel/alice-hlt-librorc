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

#include <librorc/ddl.hh>

namespace LIBRARY_NAME
{
    ddl::ddl
    (
        link *link
    )
    {
        m_link = link;
    }

    ddl::~ddl()
    {
        m_link = NULL;
    }

    bool
    ddl::linkFull()
    {
        return ( (m_link->ddlReg(RORC_REG_DDL_CTRL) & (1<<4)) != 0 );
    }

    void
    ddl::setReset
    (
        uint32_t value
    )
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<31);
        ddlctrl |= ((value&1)<<31);
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    uint32_t
    ddl::getReset()
    {
        return (m_link->ddlReg(RORC_REG_DDL_CTRL)>>31);
    }


    uint32_t
    ddl::getDeadtime()
    {
        return m_link->ddlReg(RORC_REG_DDL_DEADTIME);
    }

    void
    ddl::clearDeadtime()
    {
        m_link->setDdlReg(RORC_REG_DDL_DEADTIME, 0);
    }

    void
    ddl::setEnable
    (
        uint32_t value
    )
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<0); // clear previous value
        ddlctrl |= (value&1); // clear previous value
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }

    uint32_t
    ddl::getEnable()
    {
        return (m_link->ddlReg(RORC_REG_DDL_CTRL) & 1);
    }

    uint32_t
    ddl::getEventcount()
    {
        return m_link->ddlReg(RORC_REG_DDL_EC);
    }

    void
    ddl::clearEventcount()
    {
        m_link->setDdlReg(RORC_REG_DDL_EC, 0);
    }
}
