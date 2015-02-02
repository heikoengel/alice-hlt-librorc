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

#include <librorc/siu.hh>
#include <librorc/registers.h>
#include <librorc/link.hh>

namespace LIBRARY_NAME
{

    siu::siu( link *link ) : ddl(link){}
    siu::~siu(){}

    bool
    siu::linkOpen()
    {
        // same bit as DIU LD_N
        return ((m_link->ddlReg(RORC_REG_DDL_CTRL)>>5) & 1);
    }

    uint32_t
    siu::getDdlDeadtime()
    { return m_link->ddlReg(RORC_REG_DDL_DEADTIME); }

    void
    siu::clearDdlDeadtime()
    { m_link->setDdlReg(RORC_REG_DDL_DEADTIME, 0); }

    uint32_t
    siu::getEventcount()
    { return m_link->ddlReg(RORC_REG_DDL_EC); }

    void
    siu::clearEventcount()
    { m_link->setDdlReg(RORC_REG_DDL_EC, 0); }

    uint32_t
    siu::lastFrontEndCommandWord()
    {
        return m_link->ddlReg(RORC_REG_DDL_FESTW);
    }

    void
    siu::clearLastFrontEndCommandWord()
    {
        m_link->setDdlReg(RORC_REG_DDL_FESTW, 0);
    }

    bool
    siu::isInterfaceFifoEmpty()
    {
        return ((m_link->ddlReg(RORC_REG_DDL_CTRL)>>29) & 1)==1;
    }

    bool
    siu::isInterfaceFifoFull()
    {
        return ((m_link->ddlReg(RORC_REG_DDL_CTRL)>>28) & 1)==1;
    }

    bool
    siu::isSourceEmpty()
    {
        return ((m_link->ddlReg(RORC_REG_DDL_CTRL)>>30) & 1)==1;
    }

}
