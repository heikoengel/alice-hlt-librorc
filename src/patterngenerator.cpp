/**
 * @file patterngenerator.cpp
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

#include <librorc/patterngenerator.hh>

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
        ddlctrl &= ~(1<<8); // disable PatternGenerator
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);

        uint32_t timeout = LIBRORC_PG_TIMEOUT;
        while(!done() && timeout)
        {
            usleep(100);
        }
        if(!timeout)
        {
            /**
             * TODO: Log entry PG disable timed out, there may
             * be event fragments in the FIFOs
             **/
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
