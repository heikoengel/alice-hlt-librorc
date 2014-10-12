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

#include <librorc/gtx.hh>

namespace LIBRARY_NAME {

    gtx::gtx
    (
         librorc::link *link
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

}
