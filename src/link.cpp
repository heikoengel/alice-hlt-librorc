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

#include <unistd.h>
#include <librorc/registers.h>
#include <librorc/link.hh>
#include <librorc/bar.hh>

namespace LIBRARY_NAME
{

#define LIBRORC_LINK_WAITFORGTX_TIMEOUT 10000

    link::link
    (
        bar      *bar,
        uint32_t  link_number
    )
    {
        m_bar         = bar;
        m_base        = (link_number + 1) * RORC_CHANNEL_OFFSET;
        m_link_number = link_number;
    };


    void
    link::setGtxReg
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32( m_base+(1<<RORC_REGFILE_GTX_SEL)+addr, data);
    }


    uint32_t
    link::gtxReg(uint32_t addr)
    {
        return m_bar->get32(m_base+(1<<RORC_REGFILE_GTX_SEL)+addr);
    }


    void
    link::setDdlReg
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32( m_base+(1<<RORC_REGFILE_DDL_SEL)+addr, data);
    }


    uint32_t
    link::ddlReg(uint32_t addr)
    {
        return m_bar->get32(m_base+(1<<RORC_REGFILE_DDL_SEL)+addr);
    }


    void
    link::setPciReg
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32((m_base + addr), data);
    }


    uint32_t
    link::pciReg
    (
        uint32_t addr
    )
    {
        return m_bar->get32(m_base+addr);
    }


    void
    link::memcopy
    (
        bar_address  target,
        const void  *source,
        size_t       num
    )
    {
        m_bar->memcopy(m_base+target, source, num);
    }


    bool
    link::isGtxDomainReady()
    {
        uint32_t gtxasynccfg = pciReg(RORC_REG_GTX_ASYNC_CFG);
        uint32_t stsmask = (1<<0) | // GTX reset
            (1<<1) | // RX Reset
            (1<<2) | // RX Reset Done
            (1<<3) | // TX Reset
            (1<<4) | // TX Reset Done
            (1<<5) | // RX PLL LKDet
            (1<<8); // GTX RX clk in Reset
        uint32_t expected_value = (0<<0) | // no GTX reset
            (0<<1) | // no RX Reset
            (1<<2) | // RX Reset Done==1
            (0<<3) | // no RX Reset
            (1<<4) | // RX Reset Done==1
            (1<<5) | // RX PLL LKDet
            (0<<8); // GTX RX clk not in Reset
        return ((gtxasynccfg & stsmask) == expected_value);
    }


    bool
    link::isDdlDomainReady()
    {
        uint32_t gtxasynccfg = pciReg(RORC_REG_GTX_ASYNC_CFG);
        uint32_t stsmask = (1<<0) | // GTX reset
            (1<<7); // DDL clk in Reset
        uint32_t expected_value = (0<<0) | // no GTX reset
            (0<<7); // DDL clk not in Reset
        return ((gtxasynccfg & stsmask) == expected_value);
    }


    bool
    link::isGtxLinkUp()
    {
        uint32_t gtxsyncsts = gtxReg(RORC_REG_GTX_CTRL);
        uint32_t stsmask = (1<<0) | // PHY is ready
            (1<<1) | // PHY is enabled enable
            (1<<5) | // GTX byteisaligned
            (1<<6) | // GTX lossofsync
            (1<<9) | // GTX rxbufstatus[2]
            (1<<11); // GTX txbufstatus[1]
        uint32_t expected_value = (1<<0) | // ready==1
            (1<<1) | // enable==1
            (1<<5) | // byteisaligned==1
            (0<<6) | // lossofsync==0
            (0<<9) | // rxbufstatus[2]==0
            (0<<11); // txbufstatus[1]==0
        return ((gtxsyncsts & stsmask) == expected_value);
    }


    uint32_t
    link::linkType()
    {
        return ((pciReg(RORC_REG_GTX_ASYNC_CFG)>>12) & 3);
    }


    int
    link::waitForGTXDomain()
    {
        bool ready = isGtxDomainReady();
        uint32_t timeout = LIBRORC_LINK_WAITFORGTX_TIMEOUT;
        while( !ready && timeout!=0 )
        {
            usleep(100);
#ifndef MODELSIM
            // don't use timeout in Simulation
            timeout--;
#endif
            ready = isGtxDomainReady();
        }
        return (timeout==0) ? -1 : 0;
    }


    void
    link::setFlowControlEnable
    (
        uint32_t enable
    )
    {
        uint32_t ddlctrl = ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<1); // clear bit
        ddlctrl |= ((enable & 1)<<1); // set new value
        setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    void
    link::setChannelActive
    (
        uint32_t active
    )
    {
        uint32_t ddlctrl = ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<3); // clear bit
        ddlctrl |= ((active & 1)<<3); // set new value
        setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    bool
    link::channelIsActive()
    {
        return( (ddlReg(RORC_REG_DDL_CTRL) & (1<<3)) != 0 );
    }


    bool
    link::flowControlIsEnabled()
    {
        return( (ddlReg(RORC_REG_DDL_CTRL) & (1<<1)) != 0 );
    }


    bool
    link::patternGeneratorAvailable()
    {
        return( (ddlReg(RORC_REG_DDL_CTRL) & (1<<15)) != 0 );
    }


    bool
    link::ddr3DataReplayAvailable()
    {
        return( (ddlReg(RORC_REG_DDL_CTRL) & (1<<24)) != 0 );
    }


    void link::setDefaultDataSource()
    { setDataSourceMux(0); }


    void link::setDataSourceDiu()
    { setDataSourceMux(0); }


    void link::setDataSourcePcie()
    { setDataSourceMux(0); }


    void link::setDataSourceDdr3DataReplay()
    { setDataSourceMux(1); }


    void link::setDataSourcePatternGenerator()
    { setDataSourceMux(2); }

    const char* link::getDataSourceDescr()
    {
        uint32_t datasource = getDataSourceMux();
        switch (linkType()) {
            case RORC_CFG_LINK_TYPE_DIU:
                switch (datasource) {
                    case 0:
                        return "DDL";
                    case 1:
                        return "DDR";
                    case 2:
                        return "PG";
                    default:
                        return "UNKNOWN";
                }
            case RORC_CFG_LINK_TYPE_SIU:
                switch (datasource) {
                    case 0:
                    case 1:
                        return "PCI";
                    case 2:
                    case 3:
                        return "PG";
                    default:
                        return "UNKNOWN";
                }
            case RORC_CFG_LINK_TYPE_VIRTUAL:
                return "RAW";
            case RORC_CFG_LINK_TYPE_LINKTEST:
            case RORC_CFG_LINK_TYPE_IBERT:
                return "PG";
            default:
                return "UNKNOWN";
        }
    }

    bool link::fastClusterFinderAvailable() {
        return ((ddlReg(RORC_REG_DDL_CTRL) & (1<<25)) != 0);
    }

    /**************** protected ******************/

    void link::setDataSourceMux( uint32_t value )
    {
        uint32_t ddlctrl = ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(3<<16);
        ddlctrl |= ((value&3)<<16);
        setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }

    uint32_t link::getDataSourceMux()
    {
        uint32_t ddlctrl = ddlReg(RORC_REG_DDL_CTRL);
        return ((ddlctrl >> 16) & 3);
    }

}
