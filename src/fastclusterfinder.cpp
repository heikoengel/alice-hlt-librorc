/**
 * @file fastclusterfinder.cpp
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

#include <librorc/fastclusterfinder.hh>

namespace LIBRARY_NAME
{
    fastclusterfinder::fastclusterfinder( link *link )
    {
        m_link = link;
    }

    fastclusterfinder::~fastclusterfinder()
    {
        m_link = NULL;
    }

    /****************************************************
     * FastClusterFinder Basic Controls
     ***************************************************/
    void
    fastclusterfinder::enable()
    {
        uint32_t ddlctrl = m_link->GTX(RORC_REG_DDL_CTRL);
        ddlctrl |= (1<<8);
        m_link->setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }

    void
    fastclusterfinder::disable()
    {
        uint32_t ddlctrl = m_link->GTX(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<8);
        m_link->setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }

    bool
    fastclusterfinder::isEnabled()
    {
        return (((m_link->GTX(RORC_REG_DDL_CTRL)>>8) & 1) != 0);
    }


    /****************************************************
     * FastClusterFinder Configuration/Status
     ***************************************************/
    void
    fastclusterfinder::setSinglePadSuppression
    (
        int spSupprValue
    )
    {
        /** TODO **/
    }
}
