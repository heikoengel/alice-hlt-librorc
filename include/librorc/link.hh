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

#ifndef LIBRORC_LINK_H
#define LIBRORC_LINK_H

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>
#include <sstream>
#include <fstream>

#define LIBRORC_LINK_DDL_TIMEOUT 10000

#define LIBRORC_LINK_CMD_DIU_RCIFSTW 0x00000001
#define LIBRORC_LINK_CMD_SIU_RCIFSTW 0x00000002
#define LIBRORC_LINK_CMD_FEE_EOBTR 0x000000b4
#define LIBRORC_LINK_CMD_FEE_RDYRX 0x00000014
#define LIBRORC_LINK_CMD_DIU_LINK_RST 0x000000a1
#define LIBRORC_LINK_CMD_SIU_RST 0x00000082
#define LIBRORC_LINK_CMD_DIU_LINK_INIT 0x000000b1

/**
 * TODO: this is a copy of include/librorc/event_sanity_checker.hh:51
 **/
#define PG_PATTERN_INC    0 /** Increment value by 1 */
#define PG_PATTERN_DEC    2 /** Decrement value by 1 */
#define PG_PATTERN_SHIFT  1 /** Shifts the value to the left, the leftmost bit is inserted on the right side */
#define PG_PATTERN_TOGGLE 3 /** Toggles between the value and the negated value : 0x000000A5 -> 0xffffff5A */

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



namespace LIBRARY_NAME
{
    class bar;
    class device;

    /**
     * @brief link class, TODO: rename & cleanup!
     **/
    class link
    {
        public:
            link
            (
                bar      *bar,
                uint32_t  link_number
            );

            virtual ~link(){};


            /**
             * get BAR
             * @return bound librorc::bar
             **/
            bar *getBar()
            {
                return m_bar;
            }

            uint32_t base()
            {
                return m_base;
            }

            uint32_t linkNumber()
            { return m_link_number; }


            /**********************************************************
             *             Low Level Channel Access
             * *******************************************************/

            /**
             * set DW in GTX Domain
             * @param addr address in GTX component
             * @param data data to be written
             **/
            void
            setGtxReg
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from GTX Domain
             * @param addr address in GTX component
             * @return data read from GTX domain
             **/
            uint32_t gtxReg(uint32_t addr);

            /**
             * set DW in DDL Domain
             * @param addr address in DDL component
             * @param data data to be written
             **/
            void
            setDdlReg
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from DDL Domain
             * @param addr address in DDL component
             * @return data read from DDL domain
             **/
            uint32_t ddlReg(uint32_t addr);

            /**
             * set DW in Packtizer
             * @param addr address in PKT component
             * @param data data to be written
             **/
            void
            setPciReg
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from Packtizer
             * @param addr address in PKT component
             * @return data read from PKT
             **/
            uint32_t pciReg(uint32_t addr);

            /**
             * get GTX clock domain status
             * @return TRUE if up and running, FALSE if down
             * */
            bool isGtxDomainReady();

            /**
             * get GTX Link Status
             * @return TRUE if up and running, FALSE if down
             * */
            bool isGtxLinkUp();

            /**
             * wait for GTX domain to be ready read asynchronous GTX status
             * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
             * & !gtx_in_rst
            **/
            void waitForGTXDomain();



            /**********************************************************
             *             To be moved to dma_channel
             * *******************************************************/

            /** clear DMA stall count */
            void clearDmaStallCount();

            /** clear Event Count */
            void clearEventCount();

            void clearAllDmaCounters();

            uint32_t dmaStallCount();

            uint32_t dmaNumberOfEventsProcessed();

            /**
             * get number of Scatter Gather entries for the Event buffer
             * @return number of entries
             **/
            uint32_t getEBDMnSGEntries();

            /**
             * get number of Scatter Gather entries for the Report buffer
             * @return number of entries
             **/
            uint32_t getRBDMnSGEntries();

            /**
             * get buffer size set in EBDM. This returns the size of the
             * DMA buffer set in the DMA enginge and has to be the physical
             * size of the associated DMA buffer.
             * @return buffer size in bytes
             **/
            uint64_t getEBSize();

            /**
             * get buffer size set in RBDM. As the RB is not overmapped this size
             * should be equal to the sysfs file size and buf->getRBSize()
             * @return buffer size in bytes
             **/
            uint64_t getRBSize();

            /**
             * kill DMA engine.
             **/
            void disableDmaEngine();

            /**
             * check if DMA engine is running.
             * @return true if running, false if stopped
             **/
            bool dmaEngineIsActive();

            void clearBDMPtrMatchFlags();




            /**********************************************************
             *             Low Level GTX Status/Control
             * *******************************************************/

            /** clear all above counters */
            void clearAllGtxErrorCounters();

            /**
             * get link type
             * @return link type. Possible values are
             * RORC_CFG_LINK_TYPE_XYZ, where XYZ can be
             * DIU, SIU, VIRTUAL or LINKTEST
             **/
            uint32_t linkType();


            /**
             * Read from GTX DRP port
             * DRP = Dynamic Reconfiguratin Port (transciever)
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


            /**********************************************************
             *             Data Path Configuration
             *********************************************************/

            /**
             * set default datasource
             **/
            void
            setDefaultDataSource();

            /**
             * set DDR3 Data Replay as data source
             **/
            void
            setDataSourceDdr3DataReplay();


            /**
             * Enable/disable Flow Control
             * With flow control enabled the datapath respects full FIFOs
             * and throttles the data flow from source. No XOFF is sent
             * to DAQ as long as this is not enabled.
             * With flow control enabled the datapath from source
             * to DMA engine ignores any full FIFOs and will never
             * send XOFF to DAQ. This also means that DMA'ed events
             * may be incomplete.
             * @param enable 1 to enable, 0 to disable
             **/
            void
            setFlowControlEnable
            (
                uint32_t enable
            );


            /**
             * check if Flow Control is enabled
             * @return true if enabled, false if disabled
             **/
            bool
            flowControlIsEnabled();


            /**
             * set channel-active flag. Only an active channel can
             * push data into the DMA engine. A passive channel is
             * discarding all incoming data.
             * @param active 0 or 1
             **/
            void
            setChannelActive
            (
                uint32_t active
            );


            /**
             * check if channel is active
             * @return true if active, false if passive
             **/
            bool
            channelIsActive();


            /**
             * check if current firmware has a pattern genertor
             * implemented
             * @return true if available, else false
             **/
            bool
            patternGeneratorAvailable();


            /**
             * check if current firmware has DDR3 Data Replay
             * implemented for current link.
             * @return true if available, else false
             **/
            bool
            ddr3DataReplayAvailable();


        protected:
            bar      *m_bar;
            uint32_t  m_base;
            uint32_t  m_link_number;


            uint32_t waitForDrpDenToDeassert();

            void
            drpSetPllConfigA
            (
                uint8_t  value,
                uint8_t& n1_reg,
                uint8_t& n2_reg,
                uint8_t& d_reg
            );

            void
            drpSetPllConfigMRegister
            (
                uint8_t  value,
                uint8_t  m_reg
            );

            void
            drpSetPllConfigClkDivider
            (
                uint8_t value,
                uint8_t bit,
                uint8_t clkdiv
            );

            void drpSetPllConfigCommon(gtxpll_settings pll);

            void
            drpSetPllConfigCpCfg
            (
                uint8_t addr,
                uint8_t value
            );
};

}

#endif /** LIBRORC_LINK_H */
