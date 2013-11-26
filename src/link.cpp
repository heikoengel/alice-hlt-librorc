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

//    if (reg==0) return 1;
//    else if (reg==1) return 2;
//    else return 4;
}

static inline
uint8_t divselout_val2reg( uint8_t val )
{
    uint8_t ret;
    ret = (val==1) ? 0 : 2;
    ret = (val==2) ? 1 : ret;
    return ret;

//    if (val==1) return 0;
//    else if (val==2) return 1;
//    else return 2;
}

static inline
uint8_t divselfb_reg2val( uint8_t reg )
{
    uint8_t ret;
    ret = (reg==0) ? 2 : 5;
    ret = (reg==2) ? 4 : ret;
    return ret;

//    if (reg==0) return 2;
//    else if (reg==2) return 4;
//    else return 5;
}

static inline
uint8_t divselfb_val2reg( uint8_t val )
{
    uint8_t ret;
    ret = (val==2) ? 0 : 3;
    ret = (val==4) ? 2 : ret;
    return ret;

//    if (val==2) return 0;
//    else if (val==4) return 2;
//    else return 3;
}

static inline
uint8_t divselfb45_reg2val( uint8_t reg )
{
    return (reg==1) ? 5 : 4 ;

//    if (reg==1) return 5;
//    else return 4;
}

static inline
uint8_t divselfb45_val2reg( uint8_t val )
{
    return (val==5) ? 1 : 0 ;
//    if (val==5) return 1;
//    else return 0;
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

//    if (reg==16) return 1;
//    else return 2;
}

static inline
uint8_t divselref_val2reg( uint8_t val )
{
    return (val==1) ? 16 : 0 ;

//    if (val==1) return 16;
//    else return 0;
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
    uint16_t mask = ((uint32_t)1<<width) - 1;
    uint16_t wval = rdval & ~(mask<<bit);     /** clear current value */
    wval |= ((data & mask)<<bit);             /** set new value */
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
    link::stallCount()
    {
        return packetizer(RORC_REG_DMA_STALL_CNT);
    }


    uint32_t
    link::numberOfEventsProcessed()
    {
        return packetizer(RORC_REG_DMA_N_EVENTS_PROCESSED);
    }


    uint16_t
    link::drpRead(uint8_t drp_addr)
    {
        uint32_t drp_status;
        uint32_t drp_cmd = (0<<24)        | //read
                           (drp_addr<<16) | //DRP addr
                           (0x00);          //data

        setPacketizer(RORC_REG_GTX_DRP_CTRL, drp_cmd);

        /** wait for drp_den to deassert */
        do
        {
            usleep(100);
            drp_status = packetizer(RORC_REG_GTX_DRP_CTRL);
        } while(drp_status & (1<<31));

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
        uint32_t drp_status;
        uint32_t drp_cmd = (1<<24)        | //write
                           (drp_addr<<16) | //DRP addr
                           (drp_data);      //data

        setPacketizer(RORC_REG_GTX_DRP_CTRL, drp_cmd);

        /** wait for drp_den to deassert */
        /** TODO: timeout! */
        do
        {
            usleep(100);
            drp_status = packetizer(RORC_REG_GTX_DRP_CTRL);
        } while(drp_status & (1<<31));

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



    uint32_t
    link::getEBDMnSGEntries()
    {
        return packetizer(RORC_REG_EBDM_N_SG_CONFIG) & 0x0000ffff;
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


    uint32_t
    link::waitForDiuStatusWord
    (
        uint32_t address
    )
    {
        uint32_t timeout = LIBRORC_LINK_DDL_TIMEOUT;
        uint32_t status;

        while( ((status = GTX(address)) == 0xffffffff) &&
                (timeout!=0) )
        {
            usleep(100);
            timeout--;
        }

        /** return 0xffffffff on timeout, value on sucess */
        return status;
    }

    int
    link::waitForDiuCommandTransmissionStatusWord()
    {
        uint32_t result = waitForDiuStatusWord(RORC_REG_DDL_CTSTW);

        if (result==0xffffffff)
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "Timeout waiting for DDL "
                    "CommandTransmissionStatusWord\n");
            return -1;
        }
        else
        {
            DEBUG_PRINTF(PDADEBUG_VALUE,
                    "waitForDiuCommandTransmissionStatusWord: "
                    "got CTSTW: %08x\n", result);
            return 0;
        }

        /**
         * TODO: check error bit 31:
         * ALICE DETECTOR DATA LINK, ALICE/96-43, p.17
         * In any cases, when the error bit in the CTSTW is set to ‘1’,
         * the DDL control software shall read-out the interface status
         * words from both interface units by sending a R&CIFST command,
         * in order to identify all the errors in the DDL!
         * */
    }

    int
    link::waitForDiuInterfaceStatusWord()
    {
        uint32_t result = waitForDiuStatusWord(RORC_REG_DDL_IFSTW);

        if (result==0xffffffff)
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "Timeout waiting for DDL "
                    "InterfaceStatusWord\n");
        }
        else
        {
            DEBUG_PRINTF(PDADEBUG_VALUE,
                    "waitForDiuInterfaceStatusWord: "
                    "got IFSTW: %08x\n", result);
        }
        return result;
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



    void
    link::printDMAState()
    {
        printf("\nPKT:\n");
        printf("#Events: 0x%08x; ", packetizer(RORC_REG_DMA_N_EVENTS_PROCESSED));
        printf("#Stall: 0x%08x; ", packetizer(RORC_REG_DMA_STALL_CNT));

        uint32_t dma_ctrl = packetizer(RORC_REG_DMA_CTRL);

        printf
        (
            "PKT_EN:%d; FIFO_RST:%d; EOE_IN_FIFO:%d; FIFO_EMPTY:%d; "
            "FIFO_PEMPTY:%d; BUSY:%d; EBDM_EN:%d, RBDM_EN:%d\n",
            (dma_ctrl)&1,
            (dma_ctrl>>1)&1,
            (dma_ctrl>>4)&1,
            (dma_ctrl>>5)&1,
            (dma_ctrl>>6)&1,
            (dma_ctrl>>7)&1,
            (dma_ctrl>>2)&1,
            (dma_ctrl>>3)&1
        );

        printf("EBDM:\n");
        printf
        (
            "EBDM rdptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_EBDM_SW_READ_POINTER_L),
            packetizer(RORC_REG_EBDM_SW_READ_POINTER_H)
        );

        printf
        (
            "EBDM wrptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_EBDM_FPGA_WRITE_POINTER_L),
            packetizer(RORC_REG_EBDM_FPGA_WRITE_POINTER_H)
        );

        printf("\n");
        printf("RBDM:\n");
        printf
        (
            "RBDM rdptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_RBDM_SW_READ_POINTER_L),
            packetizer(RORC_REG_RBDM_SW_READ_POINTER_H)
        );

        printf
        (
            "RBDM wrptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_RBDM_FPGA_WRITE_POINTER_L),
            packetizer(RORC_REG_RBDM_FPGA_WRITE_POINTER_H)
        );
    }

    /** RORC_REG_DDL_CTRL encoding:
    * [0]  : DDL Interface enable
    * [1]  : Flow-Control enable
    * [2]  : Clear DIU event length counter
    * [3]  : DIU EventStream source multiplexer:
    *        0 -> use DDL as source
    *        1 -> use PG as source
    * [4]  : LinkFullN
    * [5]  : LinkDownN
    * [6]  : DIU force XOFF
    * [8]  : PatternGenerator Enable
    * [9]  : PatternGenerator Adaptive Mode
    * [10] : PatternGenerator Continuous Mode
    * [12:11] : PG mode
    *           0 -> increment
    *           1 -> rotate left
    *           2 -> decrement
    *           3 -> invert
    * [13] : PRBS event length
    * [28] : SIU Interface FIFO full
    * [29] : SIU Interface FIFO empty
    * [30] : RORC-out busyN (DIU) / source empty (SIU)
    *
    *
    * RORC_REG_DDL_PG_WAIT_TIME
    * RORC_REG_DDL_PG_EVENT_LENGTH
    * RORC_REG_DDL_PG_NUM_EVENTS
    * RORC_REG_DDL_PG_PATTERN
    **/

    void
    link::clearDiuEventSizeCounter()
    {
        setGTX(RORC_REG_DDL_CTRL, GTX(RORC_REG_DDL_CTRL)|(1<<2));
    }


    uint32_t
    link::lastDiuFrontEndStatusWord()
    {
        return GTX(RORC_REG_DDL_FESTW);
    }

    void
    link::clearLastDiuFrontEndStatusWord()
    {
        setGTX(RORC_REG_DDL_FESTW, 0);
    }


    uint32_t
    link::lastDiuCommandTransmissionStatusWord()
    {
        return GTX(RORC_REG_DDL_CTSTW);
    }

    void
    link::clearLastDiuCommandTransmissionStatusWord()
    {
        setGTX(RORC_REG_DDL_CTSTW, 0);
    }


    uint32_t
    link::lastDiuDataTransmissionStatusWord()
    {
        return GTX(RORC_REG_DDL_DTSTW);
    }

    void
    link::clearLastDiuDataTransmissionStatusWord()
    {
        setGTX(RORC_REG_DDL_DTSTW, 0);
    }


    uint32_t
    link::lastDiuInterfaceStatusWord()
    {
        return GTX(RORC_REG_DDL_IFSTW);
    }

    void
    link::clearLastDiuInterfaceStatusWord()
    {
        setGTX(RORC_REG_DDL_IFSTW, 0);
    }

    void
    link::clearAllLastDiuStatusWords()
    {
        clearLastDiuFrontEndStatusWord();
        clearLastDiuCommandTransmissionStatusWord();
        clearLastDiuDataTransmissionStatusWord();
        clearLastDiuInterfaceStatusWord();
    }


    uint32_t
    link::ddlEventCount()
    {
        return GTX(RORC_REG_DDL_EC);
    }

    uint32_t
    link::ddlDeadtime()
    {
        return GTX(RORC_REG_DDL_DEADTIME);
    }

    void
    link::clearDdlDeadtime()
    {
        setGTX(RORC_REG_DDL_DEADTIME, 0);
    }


    void
    link::diuSendCommand
    (
        uint32_t command
    )
    {
        setGTX(RORC_REG_DDL_CMD, command);
    }



    /**********************************************************
     *             Protocol Level DDL Status and Control
     * *******************************************************/
    int
    link::sendFeeEndOfBlockTransfer()
    {
        clearLastDiuCommandTransmissionStatusWord();
        diuSendCommand(LIBRORC_LINK_CMD_FEE_EOBTR); // EOBTR to FEE
        return waitForDiuCommandTransmissionStatusWord();
    }


    int
    link::sendFeeReadyToReceive()
    {
        clearLastDiuCommandTransmissionStatusWord();
        diuSendCommand(LIBRORC_LINK_CMD_FEE_RDYRX); // RdyRx to FEE
        return waitForDiuCommandTransmissionStatusWord();
    }


    uint32_t
    link::readAndClearDiuInterfaceStatus()
    {
        clearLastDiuInterfaceStatusWord();
        diuSendCommand(LIBRORC_LINK_CMD_DIU_RCIFSTW); // R&CIFSTW to DIU
        return waitForDiuInterfaceStatusWord();
    }


    uint32_t
    link::readAndClearSiuInterfaceStatus()
    {
        clearLastDiuInterfaceStatusWord();
        diuSendCommand(LIBRORC_LINK_CMD_SIU_RCIFSTW); // R&CIFSTW to SIU
        return waitForDiuInterfaceStatusWord();
    }


    void
    link::sendDiuLinkReset()
    {
        diuSendCommand(LIBRORC_LINK_CMD_DIU_LINK_RST);
    }


    void
    link::sendSiuReset()
    {
        diuSendCommand(LIBRORC_LINK_CMD_SIU_RST);
    }


    void
    link::sendDiuLinkInitialization()
    {
        diuSendCommand(LIBRORC_LINK_CMD_DIU_LINK_INIT);
    }



}
