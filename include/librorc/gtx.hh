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

#ifndef LIBRORC_GTX_H
#define LIBRORC_GTX_H

#include <librorc/defines.hh>

#define LIBRORC_GTX_FULL_RESET 1
#define LIBRORC_GTX_RX_RESET 2
#define LIBRORC_GTX_TX_RESET 8

namespace LIBRARY_NAME {

    typedef struct
    gtxpll_settings_struct
    {
        uint8_t clk25_div;
        uint8_t n1;
        uint8_t n2;
        uint8_t d;
        uint8_t m;
        uint8_t tx_tdcc_cfg;
        uint8_t cp_cfg;
        float refclk;
    }gtxpll_settings;


    /**
     * fPLL = fREF * N1 * N2 / M
     * fLineRate = fPLL * 2 / D
     * */
    const gtxpll_settings gtxpll_supported_cfgs[] ={
        //div,n1,n2,d, m, tdcc, cp, refclk
        {  9, 5, 2, 2, 1, 0, 0x0d, 212.50}, // 2.125 Gbps with RefClk=212.50 MHz
        { 10, 4, 2, 1, 1, 3, 0x0d, 250.00}, // 4.000 Gbps with RefClk=250.00 MHz
        {  9, 5, 2, 1, 1, 3, 0x0d, 212.50}, // 4.250 Gbps with RefClk=212.50 MHz
        {  9, 5, 5, 1, 2, 3, 0x07, 212.50}, // 5.3125 Gbps with RefClk=212.50 MHz
        {  7, 5, 2, 1, 1, 3, 0x0d, 156.25}, // 3.125 Gbps with RefClk=156.25 MHz
        //{ 10, 5, 2, 1, 1, 3, 0x0d, 250.00}, // 5.000 Gbps with RefClk=250.00 MHz
        //{  4, 5, 4, 2, 1, 0, 0x0d, 100.00}, // 2.000 Gbps with RefClk=100.00 MHz
        //{ 10, 4, 2, 2, 1, 0, 0x0d, 250.00}, // 2.000 Gbps with RefClk=250.00 MHz
        //{  5, 5, 5, 1, 2, 3, 0x07, 125.00}, // 3.125 Gbps with RefClk=125.00 MHz
    };

    class link;

    class gtx
    {

        public:
            /** constructor
             * @param link parent link instance
             **/
            gtx( link *link );

            /**
             * destructor
             **/
            ~gtx();

            uint32_t getTxDiffCtrl();
            uint32_t getTxPreEmph();
            uint32_t getTxPostEmph();
            uint32_t getRxEqMix();
            uint32_t getLoopback();
            uint32_t getReset();
            uint32_t getRxReset();
            uint32_t getTxReset();

            bool isDomainReady();
            bool isLinkUp();

            void setTxDiffCtrl(uint32_t value);
            void setTxPreEmph( uint32_t value );
            void setTxPostEmph( uint32_t value );
            void setRxEqMix( uint32_t value );
            void setLoopback( uint32_t value );
            void setReset( uint32_t value );
            void setRxReset( uint32_t value );
            void setTxReset( uint32_t value );

            void clearErrorCounters();
            uint32_t getDisparityErrorCount();
            uint32_t getRealignCount();
            uint32_t getRxNotInTableErrorCount();
            uint32_t getRxLossOfSyncErrorCount();

            int rxInitialize();


            /**
             * Read from GTX DRP port
             * @param drp_addr DRP address to read from
             * @return DRP value
             * */
            uint16_t
            drpRead(uint8_t drp_addr);

            /**
             * Write to GTX DRP port
             * @param drp_addr DRP address to write to
             * @param drp_data data to be written
             * */
            void
            drpWrite
            (
                uint8_t  drp_addr,
                uint16_t drp_data
            );

            /**
             * get current PLL configuration
             * @param ch pointer to dma_channel instance
             * @return struct gtxpll_settings
             * */
            gtxpll_settings
            drpGetPllConfig();

            /**
             * set new PLL configuration
             * @param ch pointer to dma_channel instance
             * @param pll struct gtxpll_settings with new values
             * */
            void
            drpSetPllConfig(gtxpll_settings pll);

        protected:
            librorc::link *m_link;

            uint32_t waitForDrpDenToDeassert();

            void
            drpUpdateField
            (
                 uint8_t drp_addr,
                 uint16_t field_data,
                 uint16_t field_bit,
                 uint16_t field_width
            );

    };
}
#endif
