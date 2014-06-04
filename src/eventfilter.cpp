/**
 * @file eventfilter.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-18
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

#include <librorc/eventfilter.hh>

namespace LIBRARY_NAME
{
    eventfilter::eventfilter( link *link )
    {
        m_link = link;
    }

    eventfilter::~eventfilter()
    {
        m_link = NULL;
    }

    void
    eventfilter::enable()
    {
        uint32_t filterctrl = m_link->ddlReg(RORC_REG_DDL_FILTER_CTRL);
        filterctrl |= (1<<31);
        m_link->setDdlReg(RORC_REG_DDL_FILTER_CTRL, filterctrl);
    }

    void
    eventfilter::disable()
    {
        uint32_t filterctrl = m_link->ddlReg(RORC_REG_DDL_FILTER_CTRL);
        filterctrl &= ~(1<<31);
        m_link->setDdlReg(RORC_REG_DDL_FILTER_CTRL, filterctrl);
    }

    bool
    eventfilter::isEnabled()
    {
        return (m_link->ddlReg(RORC_REG_DDL_FILTER_CTRL)>>31 != 0);
    }

    void
    eventfilter::setFilterAll
    (
        bool filter_all
    )
    {
        uint32_t filterctrl = m_link->ddlReg(RORC_REG_DDL_FILTER_CTRL);
        if(filter_all)
        { filterctrl |= (1<<24); }
        else
        { filterctrl &= ~(1<<24); }
        m_link->setDdlReg(RORC_REG_DDL_FILTER_CTRL, filterctrl);
    }

    bool
    eventfilter::filterAll()
    {
        return (((m_link->ddlReg(RORC_REG_DDL_FILTER_CTRL)>>24)&1) != 0);
    }

    void
    eventfilter::setFilterMask
    (
        uint32_t mask
    )
    {
        uint32_t filterctrl = m_link->ddlReg(RORC_REG_DDL_FILTER_CTRL);
        filterctrl &= 0xff000000;
        filterctrl |= (mask & 0x00ffffff);
        m_link->setDdlReg(RORC_REG_DDL_FILTER_CTRL, filterctrl);
    }

    uint32_t
    eventfilter::filterMask()
    {
        return (m_link->ddlReg(RORC_REG_DDL_FILTER_CTRL) & 0x00ffffff);
    }
}
