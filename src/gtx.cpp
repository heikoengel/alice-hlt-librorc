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
#include <unistd.h>
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

static uint32_t signedTap2Val(int tap, int width) {
  uint32_t value = 0;
  if (tap < 0) {
    value = (-tap);
    value |= (1 << (width-1));
  } else {
    value = tap;
  }
  return value;
}

static int signedVal2Tap(uint32_t value, int width) {
  int tap = (value & ((1 << (width-1)) - 1));
  if (value & (1 << (width - 1))) {
    tap = -tap;
  }
  return tap;
}

#define GTX_ASYNCCFG_RESET 0
#define GTX_ASYNCCFG_RXRESET 1
#define GTX_ASYNCCFG_TXRESET 3

#define GTX_RXINIT_RETRY_LIMIT 5
#define GTX_RXINIT_ALIGN_TIMEOUT 10000


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
        uint32_t reset = ((asynccfg >> GTX_ASYNCCFG_RESET) & 1) |
            (((asynccfg >> GTX_ASYNCCFG_RXRESET) & 1) << 1) |
            (((asynccfg >> GTX_ASYNCCFG_TXRESET) & 1) << 2);
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
        asynccfg &= ~( (1<<GTX_ASYNCCFG_RESET) |
                       (1<<GTX_ASYNCCFG_RXRESET) |
                       (1<<GTX_ASYNCCFG_TXRESET) );
        // set new reset values */
        asynccfg |= ((value & 1) << GTX_ASYNCCFG_RESET); // GTXreset
        asynccfg |= (((value >> 1) & 1) << GTX_ASYNCCFG_RXRESET); // RXreset
        asynccfg |= (((value >> 2) & 1) << GTX_ASYNCCFG_TXRESET); // TXreset
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    uint32_t gtx::getRxReset() {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        return ((asynccfg >> GTX_ASYNCCFG_RXRESET) & 1);
    }

    uint32_t gtx::getTxReset() {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        return ((asynccfg >> GTX_ASYNCCFG_TXRESET) & 1);
    }

    void gtx::setRxReset( uint32_t value ) {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        asynccfg &= ~(1 << GTX_ASYNCCFG_RXRESET);
        asynccfg |= ((value & 1) << GTX_ASYNCCFG_RXRESET);
        m_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, asynccfg);
    }

    void gtx::setTxReset( uint32_t value ) {
        uint32_t asynccfg = m_link->pciReg(RORC_REG_GTX_ASYNC_CFG);
        asynccfg &= ~(1<<GTX_ASYNCCFG_TXRESET);
        asynccfg |= ((value & 1) << GTX_ASYNCCFG_TXRESET);
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

    int
    gtx::rxInitialize()
    {
        bool rxInitDone = false;
        int retryCount = 0;

        do {
            setRxReset(1);
            usleep(100);
            setRxReset(0);

            // wait for alignment
            int timeout = GTX_RXINIT_ALIGN_TIMEOUT;
            while (!isLinkUp() && (timeout > 0)) {
                timeout--;
                usleep(100);
            }

            if (timeout == 0) {
                retryCount++;
                continue;
            }

            rxInitDone = true;
        } while (!rxInitDone && (retryCount < GTX_RXINIT_RETRY_LIMIT));

        if (rxInitDone) {
            clearErrorCounters();
            return retryCount;
        } else {
            return -1;
        }
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
    gtx::getRxLossOfSyncErrorCount()
    { return ((m_link->gtxReg(RORC_REG_GTX_RXNIT_RXLOS_CNT)) & 0xffff); }

    uint16_t
    gtx::drpRead(uint8_t drp_addr)
    {
        uint32_t drp_cmd = (0<<24) | (drp_addr<<16) | (0x00);
        m_link->setPciReg(RORC_REG_GTX_DRP_CTRL, drp_cmd);
        uint32_t drp_status = waitForDrpDenToDeassert();
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

    bool gtx::dfeTapOverride() {
        return (((m_link->gtxReg(RORC_REG_GTX_CTRL) >> 4) & 1) == 1);
    }

    void gtx::setDfeTapOverride(uint32_t ovrd) {
        uint32_t value = m_link->gtxReg(RORC_REG_GTX_CTRL);
        value &= ~(1<<4);
        value |= ((value & 1) << 4);
        m_link->setGtxReg(RORC_REG_GTX_CTRL, value);
    }

    gtxdfe_taps gtx::dfeTaps() {
        uint32_t value = m_link->gtxReg(RORC_REG_GTX_RXDFE);
        gtxdfe_taps taps;
        taps.tap1 = (value & 0x1f);
        taps.tap2 = signedVal2Tap(((value >> 5) & 0x1f), 5);
        taps.tap3 = signedVal2Tap(((value >> 10) & 0x0f), 4);
        taps.tap4 = signedVal2Tap(((value >> 14) & 0x0f), 4);
        return taps;
    }

    void gtx::setDfeTaps(gtxdfe_taps taps) {
        uint32_t value = m_link->gtxReg(RORC_REG_GTX_RXDFE);
        value &= ~(0x1ffff);
        value |= (taps.tap1 & 0x1f);
        value |= ((signedTap2Val(taps.tap2, 5) & 0x1f) << 5);
        value |= ((signedTap2Val(taps.tap3, 4) & 0x0f) << 10);
        value |= ((signedTap2Val(taps.tap4, 4) & 0x0f) << 14);
        m_link->setGtxReg(RORC_REG_GTX_RXDFE, value);
    }

    uint32_t gtx::dfeEyeOpening() {
        uint32_t value = m_link->gtxReg(RORC_REG_GTX_RXDFE);
        return ((value >> 21) & 0x1f) * 200 / 32;
    }

    void gtx::setDrpRxEyeScanMode(uint32_t value) {
        drpUpdateField(0x2e, value, 9, 2);
    }

    uint32_t gtx::drpRxEyeScanMode() {
        return ((drpRead(0x2e) >> 9) & 0x3);
    }

    uint32_t gtx::dfeClockDelayAdjustPhase() {
        return ((m_link->gtxReg(RORC_REG_GTX_RXDFE) >> 26) & 0x3f) * 64 / 90;
    }

    uint32_t gtx::dfeSensCal() {
        return ((m_link->gtxReg(RORC_REG_GTX_RXDFE) >> 18) & 0x07);
    }

    uint32_t gtx::drpRxEyeOffset() {
        return (drpRead(0x2d) >> 8);
    }

    void gtx::setDrpRxEyeOffset(uint32_t value) {
        drpUpdateField(0x2d, value, 8, 8);
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

}
