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
#include <unistd.h> //usleep

#include <librorc/patterngenerator.hh>
#include <librorc/link.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME
{
    patterngenerator::patterngenerator
    (
        link *link
    )
    {
        m_link = link;
    }


    patterngenerator::~patterngenerator()
    {
        m_link = NULL;
    }


    void
    patterngenerator::enable()
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl |= (1<<8); // enable PatternGenerator
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    void
    patterngenerator::disable()
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        if( ddlctrl & (1<<8) )
        {
            ddlctrl &= ~(1<<8); // disable PatternGenerator
            m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);

            uint32_t timeout = LIBRORC_PG_TIMEOUT;
            while(!done() && timeout)
            {
                usleep(100);
                timeout--;
            }
            if(!timeout)
            {
                /**
                 * TODO: Log entry PG disable timed out, there may
                 * be event fragments in the FIFOs
                 **/
            }
        }
    }

    void
    patterngenerator::resetEventId()
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        // set eventid_rst
        ddlctrl |= (1<<9);
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
        // release eventid_rst
        ddlctrl &= ~(1<<9);
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    void
    patterngenerator::setStaticEventSize
    (
        uint32_t eventSize
    )
    {
        m_link->setDdlReg(RORC_REG_DDL_PG_EVENT_LENGTH, eventSize);
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<13); // static event size
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    void
    patterngenerator::setPrbsSize
    (
        uint16_t prbs_min_size,
        uint32_t prbs_max_size_mask
    )
    {
        uint32_t eventSize = (prbs_max_size_mask & 0xffff0000) |
            prbs_min_size;
        m_link->setDdlReg(RORC_REG_DDL_PG_EVENT_LENGTH, eventSize);
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl |= (1<<13); // PRBS event size
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    void
    patterngenerator::configureMode
    (
        uint32_t patternMode,
        uint32_t initialPattern,
        uint32_t numberOfEvents
    )
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        /**
         * allow only implemented PatternGenerator Modes:
         * PG_PATTERN_INC, PG_PATTERN_DEC,
         * PG_PATTERN_SHIFT, PG_PATTERN_TOGGLE
         * TODO: report/break on invalid mode
         **/

        ddlctrl &= ~(3<<11); // clear current mode setting
        if( patternMode==PG_PATTERN_DEC ||
                patternMode==PG_PATTERN_SHIFT ||
                patternMode==PG_PATTERN_TOGGLE )
        { ddlctrl |= ((patternMode & 3)<<11); }
        else
        { ddlctrl |= (PG_PATTERN_INC<<11); } // default to PG_PATTERN_INC


        if(numberOfEvents)
        {
            ddlctrl &= ~(1<<10); // disable continuous mode
            m_link->setDdlReg(RORC_REG_DDL_PG_NUM_EVENTS, numberOfEvents);
        }
        else
        {
            ddlctrl |= (1<<10); // enable continuous mode
        }
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    bool
    patterngenerator::done()
    {
        return (((m_link->ddlReg(RORC_REG_DDL_CTRL)>>14) & 1) == 1);
    }


    void
    patterngenerator::useAsDataSource()
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(3<<16);
        ddlctrl |= (2<<16); // set MUX to 2
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }
}
