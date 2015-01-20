/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
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
 *     * Neither the name of University Frankfurt, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include <pda.h>
#include <librorc/gtx.hh>
#include <librorc/link.hh>
#include <librorc/registers.h>

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
{ return (reg==1) ? 5 : 4 ; }

static inline
uint8_t divselfb45_val2reg( uint8_t val )
{ return (val==5) ? 1 : 0 ; }

static inline
uint8_t clk25div_reg2val( uint8_t reg )
{ return reg+1; }

static inline
uint8_t clk25div_val2reg( uint8_t val )
{ return val-1; }

static inline
uint8_t divselref_reg2val( uint8_t reg )
{ return (reg==16) ? 1 : 2 ; }

static inline
uint8_t divselref_val2reg( uint8_t val )
{ return (val==1) ? 16 : 0 ; }



namespace LIBRARY_NAME {

    gtx::gtx
    (
         link *link
    )
    { m_link = link; }

    gtx::~gtx()
    { m_link = NULL; }

    uint32_t
    gtx::getTxDiffCtrl()
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        return ((asynccfg >> 16) & 0xf);
    }

    uint32_t
    gtx::getTxPreEmph()
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        return ((asynccfg >> 20) & 0xf);
    }

    uint32_t
    gtx::getTxPostEmph()
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        return ((asynccfg >> 24) & 0x1f);
    }

    uint32_t
    gtx::getRxEqMix()
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        return ((asynccfg >> 29) & 0x07);
    }

    uint32_t
    gtx::getLoopback()
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        return ((asynccfg >> 9) & 0x07);
    }

    uint32_t
    gtx::getReset()
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        // asynccfg reset bits:
        // bit0: GTX reset
        // bit1: RX reset
        // bit3: TX reset
        // return as {TX,RX,GTX}
        uint32_t reset = (asynccfg & 0x3) | ((asynccfg >> 1) & (1 << 2));
        return reset;
    }

    void
    gtx::setTxDiffCtrl
    (
         uint32_t value
    )
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        asynccfg &= ~(0xf << 16);
        asynccfg |= ((value & 0xf) << 16);
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    void
    gtx::setTxPreEmph
    (
         uint32_t value
    )
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        asynccfg &= ~(0xf << 20);
        asynccfg |= ((value & 0xf) << 20);
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    void
    gtx::setTxPostEmph
    (
         uint32_t value
    )
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        asynccfg &= ~(0xf << 24);
        asynccfg |= ((value & 0xf) << 24);
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    void
    gtx::setRxEqMix
    (
        uint32_t value
    )
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        asynccfg &= ~(0x7 << 29);
        asynccfg |= ((value & 0x7) << 29);
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    void
    gtx::setLoopback
    (
        uint32_t value
    )
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        asynccfg &= ~(0x7 << 9);
        asynccfg |= ((value & 0x7) << 9);
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    void
    gtx::setReset
    (
        uint32_t value
    )
    {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        // clear previous reset bits (0,1,3)
        asynccfg &= ~(0x0000000b);
        // set new reset values */
        asynccfg |= (value & 1); // GTXreset
        asynccfg |= (((value >> 1) & 1) << 1); // RXreset
        asynccfg |= (((value >> 2) & 1) << 3); // TXreset
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    bool
    gtx::isDomainReady()
    {
        return m_link->isGtxDomainReady();
    }

    bool
    gtx::isLinkUp()
    {
        return m_link->isGtxLinkUp();
    }

    void
    gtx::clearErrorCounters()
    {
        m_link->setGtxReg(RORC_REG_GTX_DISPERR_REALIGN_CNT, 0);
        m_link->setGtxReg(RORC_REG_GTX_RXNIT_RXLOS_CNT, 0);
        m_link->setGtxReg(RORC_REG_GTX_ERROR_CNT, 0);
    }

    uint32_t
    gtx::getDisparityErrorCount()
    { return ((m_link->gtxReg(RORC_REG_GTX_DISPERR_REALIGN_CNT))>>16); }

    uint32_t
    gtx::getRealignCount()
    { return ((m_link->gtxReg(RORC_REG_GTX_DISPERR_REALIGN_CNT)) & 0xffff); }

    uint32_t
    gtx::getRxNotInTableErrorCount()
    { return ((m_link->gtxReg(RORC_REG_GTX_RXNIT_RXLOS_CNT))>>16); }

    uint32_t
    gtx::getRxLossOfSignalErrorCount()
    { return ((m_link->gtxReg(RORC_REG_GTX_RXNIT_RXLOS_CNT)) & 0xffff); }

    uint16_t
    gtx::drpRead(uint8_t drp_addr)
    {
        uint32_t drp_cmd = (0<<24) | (drp_addr<<16) | (0x00);
        m_link->setPciReg(RORC_REG_GTX_DRP_CTRL, drp_cmd);
        uint32_t drp_status = waitForDrpDenToDeassert();

        DEBUG_PRINTF( PDADEBUG_CONTROL_FLOW,
            "drpRead(%x)=%04x\n", drp_addr, (drp_status & 0xffff) );
        return (drp_status & 0xffff);
    }

    void
    gtx::drpWrite
    (
        uint8_t  drp_addr,
        uint16_t drp_data
    )
    {
        uint32_t drp_cmd = (1<<24) | (drp_addr<<16) | (drp_data);
        m_link->setPciReg(RORC_REG_GTX_DRP_CTRL, drp_cmd);
        waitForDrpDenToDeassert();
        DEBUG_PRINTF( PDADEBUG_CONTROL_FLOW,
            "drpWrite(%x, %04x)\n", drp_addr, drp_data );
    }

    gtxpll_settings
    gtx::drpGetPllConfig()
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

        drpdata         = drpRead(0x1e);
        pll.cp_cfg      = ((drpdata>>8) & 0xff);

        // Frequency = refclk_freq*gtx_n1*gtx_n2/gtx_m*2/gtx_d;
        return pll;
    }

    void
    gtx::drpSetPllConfig(gtxpll_settings pll)
    {
        uint8_t  n1_reg   = divselfb45_val2reg(pll.n1);
        uint8_t  n2_reg   = divselfb_val2reg(pll.n2);
        uint8_t  d_reg    = divselout_val2reg(pll.d);
        uint8_t  m_reg    = divselref_val2reg(pll.m);
        uint8_t  clkdiv   = clk25div_val2reg(pll.clk25_div);

        // TXPLL
        // set TXPLL_DIVSEL_FB45/N1: addr 0x1f bit [6]
        drpUpdateField(0x1f, n1_reg, 6, 1);
        // set TXPLL_DIVSEL_FB/N2: addr 0x1f bits [5:1]
        drpUpdateField(0x1f, n2_reg, 1, 5);
        // set TXPLL_DIVSEL_OUT/D: addr 0x1f bits [15:14] */
        drpUpdateField(0x1f, d_reg, 14, 2);
        // set TXPLL_DIVSEL_REF/M: addr 0x20, bits [5:1]
        drpUpdateField(0x20, m_reg, 1, 5);
        // set TX_CLK25_DIVIDER: addr 0x23, bits [14:10]
        drpUpdateField(0x23, clkdiv, 10, 5);
        // set TX_CP_CFG: addr 0x1e, bits[15:8]
        drpUpdateField(0x1e, pll.cp_cfg, 8, 8);

        // RXPLL
        // set RXPLL_DIVSEL_FB45/N1: addr 0x1b bit [6]
        drpUpdateField(0x1b, n1_reg, 6, 1);
        // set RXPLL_DIVSEL_FB/N2: addr 0x1b bits [5:1]
        drpUpdateField(0x1b, n2_reg, 1, 5);
        // set RXPLL_DIVSEL_OUT/D: addr 0x1b bits [15:14] */
        drpUpdateField(0x1b, d_reg, 14, 2);
        // set RXPLL_DIVSEL_REF/M: addr 0x1c, bits [5:1]
        drpUpdateField(0x1c, m_reg, 1, 5);
        // set RX_CLK25_DIVIDER: addr 0x17, bits [9:5]
        drpUpdateField(0x17, clkdiv, 5, 5);
        // set RX_CP_CFG: addr 0x1a, bits[15:8]
        drpUpdateField(0x1a, pll.cp_cfg, 8, 8);

        // Common
        // set TX_TDCC_CFG: addr 0x39, bits [15:14]
        drpUpdateField(0x39, pll.tx_tdcc_cfg, 14, 2);
    }

    double
    gtx::dfeEye()
    {
        return ((m_link->gtxReg(RORC_REG_GTX_RXDFE)>>21 & 0x1f)*200.0/31.0);
    }


    /**************************************************************************
     *                            protected
     *************************************************************************/

    uint32_t
    gtx::waitForDrpDenToDeassert()
    {
        uint32_t drp_status;
        do
        {
            usleep(100);
            drp_status = m_link->pciReg(RORC_REG_GTX_DRP_CTRL);
        } while (drp_status & (1 << 31));
        return drp_status;
    }

    void
    gtx::drpUpdateField
    (
        uint8_t drp_addr,
        uint16_t field_data,
        uint16_t field_bit,
        uint16_t field_width
    )
    {
        // read current value at drp_addr
        uint16_t drp_data_orig = drpRead(drp_addr);
        uint16_t bitmask = (((uint32_t)1)<<field_width)-1;
        // clear previous field data
        uint16_t drp_data_new = drp_data_orig & ~(bitmask<<field_bit);
        // set new field data at field_bit
        drp_data_new |= ((field_data & bitmask)<<field_bit);
        // write new register value back to drp_addr
        if( drp_data_orig != drp_data_new )
        {
            drpWrite(drp_addr, drp_data_new);
            drpRead(0);
        }
    }
}
