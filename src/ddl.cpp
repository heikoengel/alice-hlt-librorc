/**
 * @file ddl.cpp
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
        return ( (m_link->DDL(RORC_REG_DDL_CTRL) & (1<<4)) != 0 );
    }

    void
    ddl::setReset
    (
        uint32_t value
    )
    {
        uint32_t ddlctrl = m_link->DDL(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<31);
        ddlctrl |= ((value&1)<<31);
        m_link->setDDL(RORC_REG_DDL_CTRL, ddlctrl);
    }


    uint32_t
    ddl::getReset()
    {
        return (m_link->DDL(RORC_REG_DDL_CTRL)>>31);
    }


    uint32_t
    ddl::getDeadtime()
    {
        return m_link->DDL(RORC_REG_DDL_DEADTIME);
    }

    void
    ddl::clearDeadtime()
    {
        m_link->setDDL(RORC_REG_DDL_DEADTIME, 0);
    }

    void
    ddl::enableInterface()
    {
        uint32_t ddlctrl = m_link->DDL(RORC_REG_DDL_CTRL);
        ddlctrl |= (1<<0); // enable DDL_IF
        m_link->setDDL(RORC_REG_DDL_CTRL, ddlctrl);
    }

    void
    ddl::disableInterface()
    {
        uint32_t ddlctrl = m_link->DDL(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<0); // disable DDL_IF
        m_link->setDDL(RORC_REG_DDL_CTRL, ddlctrl);
    }

    uint32_t
    ddl::getEventcount()
    {
        return m_link->DDL(RORC_REG_DDL_EC);
    }

    void
    ddl::clearEventcount()
    {
        m_link->setDDL(RORC_REG_DDL_EC, 0);
    }
}
