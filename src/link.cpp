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
        return( (gtxasynccfg & (1<<8)) == 0 );
    }

    void
    link::clearGtxDisparityErrorCount()
    {
        !isGtxDomainReady() ? (void)0 : setGTX(RORC_REG_GTX_DISPERR_CNT, 0);
    }

    void
    link::clearGtxRxNotInTableCount()
    {
        !isGtxDomainReady() ? (void)0 : setGTX(RORC_REG_GTX_RXNIT_CNT, 0);
    }

    void
    link::clearGtxRxLossOfSignalCount()
    {
        !isGtxDomainReady() ? (void)0 : setGTX(RORC_REG_GTX_RXLOS_CNT, 0);
    }

    void
    link::clearGtxRxByteRealignCount()
    {
        !isGtxDomainReady() ? (void)0 : setGTX(RORC_REG_GTX_RXBYTEREALIGN_CNT, 0);
    }

    void
    link::clearAllGtxErrorCounter()
    {
        clearGtxDisparityErrorCount();
        clearGtxRxNotInTableCount();
        clearGtxRxLossOfSignalCount();
        clearGtxRxByteRealignCount();
    }

    uint32_t
    link::dmaStallCount()
    {
        return packetizer(RORC_REG_DMA_STALL_CNT);
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

        uint32_t drp_status
            = waitForDrpDenToDeassert();

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

        uint16_t drp_data = 0;

        /********************* TXPLL *********************/

        drp_data = drpRead(0x1f);
        drp_data = read_modify_write(drp_data, n1_reg, 6, 1); /** set TXPLL_DIVSEL_FB45/N1: addr 0x1f bit [6] */
        drp_data = read_modify_write(drp_data, n2_reg, 1, 5); /** set TXPLL_DIVSEL_FB/N2: addr 0x1f bits [5:1] */
        drp_data = read_modify_write(drp_data, d_reg, 14, 2); /** set TXPLL_DIVSEL_OUT/D: addr 0x1f bits [15:14] */
        drpWrite(0x1f, drp_data);
        drpRead(0x0);

        /** set TXPLL_DIVSEL_REF/M: addr 0x20, bits [5:1] */
        drp_data = drpRead(0x20);
        drp_data = read_modify_write(drp_data, m_reg, 1, 5);
        drpWrite(0x20, drp_data);
        drpRead(0x0);

        /** set TX_CLK25_DIVIDER: addr 0x23, bits [14:10] */
        drp_data = drpRead(0x23);
        drp_data = read_modify_write(drp_data, clkdiv, 10, 5);
        drpWrite(0x23, drp_data);
        drpRead(0x0);

        /********************* RXPLL *********************/

        drp_data = drpRead(0x1b);
        drp_data = read_modify_write(drp_data, n1_reg, 6, 1); /** set RXPLL_DIVSEL_FB45/N1: addr 0x1b bit [6] */
        drp_data = read_modify_write(drp_data, n2_reg, 1, 5); /** set RXPLL_DIVSEL_FB/N2: addr 0x1b bits [5:1] */
        drp_data = read_modify_write(drp_data, d_reg, 14, 2); /** set RXPLL_DIVSEL_OUT/D: addr 0x1b bits [15:14] */
        drpWrite(0x1b, drp_data);
        drpRead(0x0);

        /** set RXPLL_DIVSEL_REF/M: addr 0x1c, bits [5:1] */
        drp_data = drpRead(0x1c);
        drp_data = read_modify_write(drp_data, m_reg, 1, 5);
        drpWrite(0x1c, drp_data);
        drpRead(0x0);

        /** set RX_CLK25_DIVIDER: addr 0x17, bits [9:5] */
        drp_data = drpRead(0x17);
        drp_data = read_modify_write(drp_data, clkdiv, 5, 5);
        drpWrite(0x17, drp_data);
        drpRead(0x0);


        /********************* Common *********************/

        /** TX_TDCC_CFG: addr 0x39, bits [15:14] */
        drp_data = drpRead(0x39);
        drp_data = read_modify_write(drp_data, pll.tx_tdcc_cfg, 14, 2);
        drpWrite(0x39, drp_data);
        drpRead(0x0);
    }



    void
    link::waitForCommandTransmissionStatusWord() /** (CTSTW) from DIU */
    {
        uint32_t timeout = LIBRORC_LINK_DDL_TIMEOUT;
        while( (GTX(RORC_REG_DDL_CTSTW) == 0xffffffff) &&
                (timeout!=0) )
        {
            usleep(100);
            timeout--;
        }
        if ( !timeout )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "Timeout waiting for CommandTransmissionStatusWord\n");
        }
    }

    void
    link::waitForGTXDomain()
    {
        while( (packetizer(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
        { usleep(100); }
    }


    void
    link::printDiuState()
    {
        uint32_t status = GTX(RORC_REG_DDL_CTRL);

        printf("\nDIU_IF: ");

        (status & 1)       ? printf("DIU_ON ") : printf("DIU_OFF ");
        ((status>>1)  & 1) ? printf("FC_ON ")  : printf("FC_OFF ");
        ((status>>4)  & 1) ? 0                 : printf("LF ");
        ((status>>5)  & 1) ? 0                 : printf("LD ");
        ((status>>30) & 1) ? 0                 : printf("BSY ");

        /** PG disabled or enabled */
        ((status>>8) & 1)  ? printf("PG_ON")   : printf("PG_OFF");
        ((status>>8) & 1)  ? 0                 : printf("CTSTW: %08x ", GTX(RORC_REG_DDL_CTSTW));
        ((status>>8) & 1)  ? 0                 : printf("DEADTIME: %08x ", GTX(RORC_REG_DDL_DEADTIME));
        ((status>>8) & 1)  ? 0                 : printf("EC: %08x ", GTX(RORC_REG_DDL_EC));
    }

}
