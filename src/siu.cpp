/**
 * @file siu.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-15
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

#include <librorc/siu.hh>

namespace LIBRARY_NAME
{

    siu::siu( link *link ) : ddl(link)
    {
        m_link = link;
    }

    siu::~siu()
    {
        m_link = NULL;
    }

    bool
    siu::linkOpen()
    {
        // same bit as DIU LD_N
        return ((m_link->GTX(RORC_REG_DDL_CTRL)>>5) & 1);
    }

    uint32_t
    siu::lastFrontEndCommandWord()
    {
        return m_link->GTX(RORC_REG_DDL_FESTW);
    }

    void
    siu::clearLastFrontEndCommandWord()
    {
        m_link->setGTX(RORC_REG_DDL_FESTW, 0);
    }

    bool
    siu::isInterfaceFifoEmpty()
    {
        return ((m_link->GTX(RORC_REG_DDL_CTRL)>>29) & 1)==1;
    }

    bool
    siu::isInterfaceFifoFull()
    {
        return ((m_link->GTX(RORC_REG_DDL_CTRL)>>28) & 1)==1;
    }

    bool
    siu::isSourceEmpty()
    {
        return ((m_link->GTX(RORC_REG_DDL_CTRL)>>30) & 1)==1;
    }

}
