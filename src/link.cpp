/**
 * @file
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>, Heiko Engel <hengel@cern.ch>
 * @date 2011-09-18
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
 */

#include <librorc/registers.h>
#include <librorc/link.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>

#include <pda.h>



/** Conversions between PLL values and their register representations */
static inline
uint8_t divselout_reg2val( uint8_t reg )
{
    uint8_t ret;
    ret = (reg==0) ? 1 : 4;
    ret = (reg==1) ? 2 : ret;
    return ret;
}

static inline
uint8_t divselout_val2reg( uint8_t val )
{
    uint8_t ret;
    ret = (val==1) ? 0 : 2;
    ret = (val==2) ? 1 : ret;
    return ret;
}

static inline
uint8_t divselfb_reg2val( uint8_t reg )
{
    uint8_t ret;
    ret = (reg==0) ? 2 : 5;
    ret = (reg==2) ? 4 : ret;
    return ret;
}

static inline
uint8_t divselfb_val2reg( uint8_t val )
{
    uint8_t ret;
    ret = (val==2) ? 0 : 3;
    ret = (val==4) ? 2 : ret;
    return ret;
}

static inline
uint8_t divselfb45_reg2val( uint8_t reg )
{
    return (reg==1) ? 5 : 4 ;
}

static inline
uint8_t divselfb45_val2reg( uint8_t val )
{
    return (val==5) ? 1 : 0 ;
}

static inline
uint8_t clk25div_reg2val( uint8_t reg )
{
    return reg+1;
}

static inline
uint8_t clk25div_val2reg( uint8_t val )
{
    return val-1;
}

static inline
uint8_t divselref_reg2val( uint8_t reg )
{
    return (reg==16) ? 1 : 2 ;
}

static inline
uint8_t divselref_val2reg( uint8_t val )
{
    return (val==1) ? 16 : 0 ;
}

static inline
uint16_t
read_modify_write
(
    uint16_t rdval,
    uint16_t data,
    uint8_t  bit,
    uint8_t  width
)
{
    /** Replace rdval[bit+width-1:bit] with data[width-1:0] */
    uint16_t mask  = ((uint32_t)1<<width) - 1;
    uint16_t wval  = rdval & ~(mask<<bit);     /** clear current value */
    wval          |= ((data & mask)<<bit);     /** set new value */
    return wval;
}



namespace LIBRARY_NAME
{
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
    link::setGTX
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32( m_base+(1<<RORC_DMA_CMP_SEL)+addr, data);
    }



    uint32_t
    link::GTX(uint32_t addr)
    {
        return m_bar->get32(m_base+(1<<RORC_DMA_CMP_SEL)+addr);
    }



    void
    link::setPacketizer
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32((m_base + addr), data);
    }



    uint32_t
    link::packetizer
    (
        uint32_t addr
    )
    {
        return m_bar->get32(m_base+addr);
    }



    void
    link::clearDmaStallCount()
    {
        setPacketizer(RORC_REG_DMA_STALL_CNT, 0);
    }

    void
    link::clearEventCount()
    {
        setPacketizer(RORC_REG_DMA_N_EVENTS_PROCESSED, 0);
    }

    bool
    link::isGtxDomainReady()
    {
        uint32_t gtxasynccfg = packetizer(RORC_REG_GTX_ASYNC_CFG);
        return( (gtxasynccfg & 0x00000134) == 0x00000034 );
    }

    void
    link::clearGtxDisparityErrorCount()
    {
        setGTX(RORC_REG_GTX_DISPERR_CNT, 0);
    }

    void
    link::clearGtxRxNotInTableCount()
    {
        setGTX(RORC_REG_GTX_RXNIT_CNT, 0);
    }

    void
    link::clearGtxRxLossOfSignalCount()
    {
        setGTX(RORC_REG_GTX_RXLOS_CNT, 0);
    }

    void
    link::clearGtxRxByteRealignCount()
    {
        setGTX(RORC_REG_GTX_RXBYTEREALIGN_CNT, 0);
    }

    void
    link::clearAllGtxErrorCounters()
    {
        clearGtxDisparityErrorCount();
        clearGtxRxNotInTableCount();
        clearGtxRxLossOfSignalCount();
        clearGtxRxByteRealignCount();
    }

    uint32_t
    link::linkType()
    {
        return ((packetizer(RORC_REG_GTX_ASYNC_CFG)>>12) & 3);
    }


    uint32_t
    link::dmaStallCount()
    {
        return packetizer(RORC_REG_DMA_STALL_CNT);
    }

    void
    link::clearAllDmaCounters()
    {
        clearEventCount();
        clearDmaStallCount();
    }


    uint32_t
    link::dmaNumberOfEventsProcessed()
    {
        return packetizer(RORC_REG_DMA_N_EVENTS_PROCESSED);
    }

    uint32_t
    link::waitForDrpDenToDeassert()
    {
        uint32_t drp_status;
        do
        {
            usleep(100);
            drp_status = packetizer(RORC_REG_GTX_DRP_CTRL);
        } while (drp_status & (1 << 31));
        return drp_status;
    }

    uint16_t
    link::drpRead(uint8_t drp_addr)
    {
        uint32_t drp_cmd = (0<<24)        | //read
                           (drp_addr<<16) | //DRP addr
                           (0x00);          //data

        setPacketizer(RORC_REG_GTX_DRP_CTRL, drp_cmd);

        uint32_t drp_status
            = waitForDrpDenToDeassert();

        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "drpRead(%x)=%04x\n",
            drp_addr,
            (drp_status & 0xffff)
        );

        return (drp_status & 0xffff);
    }



    void
    link::drpWrite
    (
        uint8_t  drp_addr,
        uint16_t drp_data
    )
    {
        uint32_t drp_cmd = (1<<24)        | //write
                           (drp_addr<<16) | //DRP addr
                           (drp_data);      //data

        setPacketizer(RORC_REG_GTX_DRP_CTRL, drp_cmd);

        waitForDrpDenToDeassert();

        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "drpWrite(%x, %04x)\n",
            drp_addr,
            drp_data
        );
    }



    gtxpll_settings
    link::drpGetPllConfig()
    {
        uint16_t drpdata;
        gtxpll_settings pll;

        drpdata         = drpRead(0x1f);
        pll.n1          = divselfb45_reg2val((drpdata>>6)&0x1);
        pll.n2          = divselfb_reg2val((drpdata>>1)&0x1f);
        pll.d           = divselout_reg2val((drpdata>>14)&0x3);

        drpdata         = drpRead(0x20);
        pll.m           = divselref_reg2val((drpdata>>1)&0x1f);

        drpdata         = drpRead(0x23);
        pll.clk25_div   = clk25div_reg2val((drpdata>>10)&0x1f);

        drpdata         = drpRead(0x39);
        pll.tx_tdcc_cfg = (drpdata>>14) & 0x03;

        /**Frequency = refclk_freq*gtx_n1*gtx_n2/gtx_m*2/gtx_d; */
        return pll;
    }

    void
    link::drpSetPllConfigA
    (
        uint8_t  value,
        uint8_t& n1_reg,
        uint8_t& n2_reg,
        uint8_t& d_reg
    )
    {
        uint16_t drp_data = 0;
        uint16_t drp_data_orig = drpRead(value);
        /** set TXPLL_DIVSEL_FB45/N1: addr 0x1f bit [6] */
        drp_data = read_modify_write(drp_data_orig, n1_reg, 6, 1);
        /** set TXPLL_DIVSEL_FB/N2: addr 0x1f bits [5:1] */
        drp_data = read_modify_write(drp_data, n2_reg, 1, 5);
        /** set TXPLL_DIVSEL_OUT/D: addr 0x1f bits [15:14] */
        drp_data = read_modify_write(drp_data, d_reg, 14, 2);
        if (drp_data != drp_data_orig )
        {
            drpWrite(value, drp_data);
            drpRead(0x0);
        }
    }

    void
    link::drpSetPllConfigMRegister
    (
        uint8_t  value,
        uint8_t  m_reg
    )
    {
        uint16_t drp_data = 0;
        uint16_t drp_data_orig = drpRead(value);
        drp_data = read_modify_write(drp_data_orig, m_reg, 1, 5);
        if (drp_data != drp_data_orig )
        {
            drpWrite(value, drp_data);
            drpRead(0x0);
        }
    }

    void
    link::drpSetPllConfigClkDivider
    (
        uint8_t value,
        uint8_t bit,
        uint8_t clkdiv
    )
    {
        uint16_t drp_data = 0;
        uint16_t drp_data_orig = drpRead(value);
        drp_data = read_modify_write(drp_data_orig, clkdiv, bit, 5);
        if (drp_data != drp_data_orig )
        {
            drpWrite(value, drp_data);
            drpRead(0x0);
        }
    }

    void
    link::drpSetPllConfigCommon
    (
        gtxpll_settings pll
    )
    {
        uint16_t drp_data = 0;
        uint16_t drp_data_orig = drpRead(0x39);
        drp_data = read_modify_write(drp_data_orig, pll.tx_tdcc_cfg, 14, 2);
        if (drp_data != drp_data_orig )
        {
            drpWrite(0x39, drp_data);
            drpRead(0x0);
        }
    }

    /**
     * set new PLL configuration
     * @param ch pointer to dma_channel instance
     * @param pll struct gtxpll_settings with new values
     * */
    void
    link::drpSetPllConfig(gtxpll_settings pll)
    {
        uint8_t  n1_reg   = divselfb45_val2reg(pll.n1);
        uint8_t  n2_reg   = divselfb_val2reg(pll.n2);
        uint8_t  d_reg    = divselout_val2reg(pll.d);
        uint8_t  m_reg    = divselref_val2reg(pll.m);
        uint8_t  clkdiv   = clk25div_val2reg(pll.clk25_div);

        /********************* TXPLL *********************/

        drpSetPllConfigA(0x1f, n1_reg, n2_reg, d_reg);

        /** set TXPLL_DIVSEL_REF/M: addr 0x20, bits [5:1] */
        drpSetPllConfigMRegister(0x20, m_reg);

        /** set TX_CLK25_DIVIDER: addr 0x23, bits [14:10] */
        drpSetPllConfigClkDivider(0x23, 10, clkdiv);

        /********************* RXPLL *********************/

        drpSetPllConfigA(0x1b, n1_reg, n2_reg, d_reg);

        /** set RXPLL_DIVSEL_REF/M: addr 0x1c, bits [5:1] */
        drpSetPllConfigMRegister(0x1c, m_reg);

        /** set RX_CLK25_DIVIDER: addr 0x17, bits [9:5] */
        drpSetPllConfigClkDivider(0x17, 5, clkdiv);

        /********************* Common *********************/

        /** TX_TDCC_CFG: addr 0x39, bits [15:14] */
        drpSetPllConfigCommon(pll);
    }



    uint32_t
    link::getRBDMnSGEntries()
    {
        return packetizer(RORC_REG_RBDM_N_SG_CONFIG) & 0x0000ffff;
    }

    uint64_t
    link::getEBSize()
    {
        return ((uint64_t)packetizer(RORC_REG_EBDM_BUFFER_SIZE_H) << 32) +
               (uint64_t)packetizer(RORC_REG_EBDM_BUFFER_SIZE_L);
    }


    uint64_t
    link::getRBSize()
    {
        return ((uint64_t)packetizer(RORC_REG_RBDM_BUFFER_SIZE_H) << 32) +
               (uint64_t)packetizer(RORC_REG_RBDM_BUFFER_SIZE_L);
    }

    void
    link::disableDmaEngine()
    {
        /**
         * disable EBDM + RBDM
         * disable Packetizer
         * Reset D_FIFO
         * Reset MaxPayload
         **/
        setPacketizer(RORC_REG_DMA_CTRL, 0X00000002);
    }


    void
    link::waitForGTXDomain()
    {
        /**
         * poll asynchronous GTX status
         * wait for rxresetdone & txresetdone & rxplllkdet &
         * txplllkdet & !gtx_in_rst
         *
         * TODO:
         * - add timeout
         * - return/handle/report timeout error
         * */
        while( (packetizer(RORC_REG_GTX_ASYNC_CFG) & 0x134) != 0x034 )
        { usleep(100); }
    }


    /**********************************************************
     *             Fast Cluster Finder Interfacing
     * *******************************************************/

    // protected
    void
    link::fcfWriteMappingRamEntry
    (
        uint32_t addr,
        uint32_t data
    )
    {
        setGTX(RORC_REG_FCF_RAM_DATA, data);
        setGTX(RORC_REG_FCF_RAM_CTRL, addr);
    }


    // protected
    uint32_t
    link::fcfHexstringToUint32
    (
        string line
    )
    {
        uint32_t hexval;
        stringstream ss;
        ss << hex << line;
        ss >> hexval;
        return hexval;
    }



    void
    link::fcfLoadMappingRam
    (
        const char *fname
    )
    {
        ifstream memfile(fname);
        if ( !memfile.is_open() )
        {
            cout << "Failed to open mapping file" << endl;
            abort();
        }

        string line;
        uint32_t i = 0;

        while ( getline(memfile, line) )
        {
            if ( i>4095 )
            {
                DEBUG_PRINTF(PDADEBUG_ERROR, "Mapping file has more " \
                        "than 4096 entries - skipping remaining lines");
                break;
            }

            uint32_t hexval = fcfHexstringToUint32(line);
            fcfWriteMappingRamEntry(i, hexval);
            i++;
        }

        if ( i<4096 )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Mapping file has less " \
                    "than 4096 entries - filling remaining lines");
            while( i<4096 )
            {
                fcfWriteMappingRamEntry(i, 0);
                i++;
            }
        }
    }

    void
    link::enableFlowControl()
    {
        uint32_t ddlctrl = GTX(RORC_REG_DDL_CTRL);
        ddlctrl |= (1<<1); // enable flow control
        setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }

    void
    link::disableFlowControl()
    {
        uint32_t ddlctrl = GTX(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<1); // disable flow control
        setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }

    void
    link::enableFcf()
    {
        // enable FCF (same bit as PG_ENABLE)
        uint32_t ddlctrl = GTX(RORC_REG_DDL_CTRL);
        ddlctrl |= (1<<8); // enable PatternGenerator
        setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }

    void
    link::disableFcf()
    {
        uint32_t ddlctrl = GTX(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<8); // disable FCF (same bit as PG_ENABLE)
        setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }

    void
    link::setDataSourceDdr3DataReplay()
    {
        // set MUX to DDR3 DataReplay:
        // same as PatternGenerator for HLT_IN & HWTEST
        uint32_t ddlctrl = GTX(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(3<<16);
        ddlctrl |= (1<<16); // set MUX to 1
        setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }


    void
    link::configureDdr3DataReplayChannel
    (
        uint32_t ddr3_start_address
    )
    {
        uint32_t ch_cfg = GTX(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        ch_cfg &= ~(0xfffffffc); // clear [31:2]
        ch_cfg |= (ddr3_start_address & 0xfffffffc); //set start_addr[31:2]
        setGTX(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, ch_cfg);
    }


    void
    link::enableDdr3DataReplayChannel()
    {
        uint32_t ch_cfg = GTX(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        ch_cfg |= (1<<1) | // continuous
            (1<<0); //enable
        setGTX(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, ch_cfg);
    }

    void
    link::disableDdr3DataReplayChannel()
    {
        uint32_t ch_cfg = GTX(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL);
        ch_cfg &= ~(1<<0); //disable
        setGTX(RORC_REG_DDR3_DATA_REPLAY_CHANNEL_CTRL, ch_cfg);
    }

}
