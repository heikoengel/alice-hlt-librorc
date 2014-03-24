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

typedef struct
gtxpll_settings_struct
{
    uint8_t clk25_div;
    uint8_t n1;
    uint8_t n2;
    uint8_t d;
    uint8_t m;
    uint8_t tx_tdcc_cfg;
    float refclk;
}gtxpll_settings;



namespace LIBRARY_NAME
{
class bar;
class device;

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


            /**********************************************************
             *             Low Level Channel Access
             * *******************************************************/

            /**
             * set DW in GTX Domain
             * @param addr address in GTX component
             * @param data data to be writtem
             **/
            void
            setGTX
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from GTX Domain
             * @param addr address in GTX component
             * @return data read from GTX
             **/
            uint32_t GTX(uint32_t addr);

            /**
             * set DW in Packtizer
             * @param addr address in PKT component
             * @param data data to be writtem
             **/
            void
            setPacketizer
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from Packtizer
             * @param addr address in PKT component
             * @return data read from PKT
             **/
            uint32_t packetizer(uint32_t addr);

            /**
             * get GTX clock domain status
             * @return TRUE is up and running, FALSE if down
             * */
            bool isGtxDomainReady();

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

            uint32_t dmaStallCount();

            uint32_t dmaNumberOfEventsProcessed();

            /**
             * Printout the state of the DMA engine to the console
             * */
            void printDMAState();

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



            /**********************************************************
             *             Low Level GTX Status/Control
             * *******************************************************/

            /**
             * GTX Error Counters
             * */
            void clearGtxDisparityErrorCount();
            void clearGtxRxNotInTableCount();
            void clearGtxRxLossOfSignalCount();
            void clearGtxRxByteRealignCount();
            void clearAllGtxErrorCounters();


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
             *             Low Level DDL Status and Control
             * *******************************************************/

            /**
             * DIU Interface internal counter providing the reported
             * event size (riD[31:12]) to DMA engine on each EndOfEvent.
             * Clearing this counter should only be required after a
             * crash. No doing so may only result in in a mismatch of
             * calculated and reported event size. No data gets lost.
             * */
            void
            clearDiuEventSizeCounter();


            /**
             * HLT_IN: get last FrontEndStatusWord received on DIU
             * This is equivalent to lastSiuFronEndCommand on HLT_OUT
             * @return FESTW:
             * [31]    Error
             * [30:12] Front-End Status
             * [11:8]  Transaction ID
             * [7:6]   "01"
             * [5]     End Of Data Block (EODB)
             * [4:0]   "00100"
             * */
            uint32_t
            lastDiuFrontEndStatusWord();

            /**
             * HLT_OUT: get last FronEndCommand received from SIU.
             * This is equivalent to lastDiuFrontEndStatusWord on HLT_IN
             * */
            uint32_t
            lastSiuFrontEndCommandWord();


            /**
             * HLT_IN: clear last CommandTransmissionStatusWord.
             * after this call the register holds 0xffffffff
             * */
            void
            clearLastDiuFrontEndStatusWord();


            /**
             * HLT_OUT: clear last Front-End Command received on SIU
             * after this call the register holds 0xffffffff
             * */
            void
            clearLastSiuFrontEndCommandWord();



            /**
             * HLT_IN: get last CommandTransmissionStatusWord from DIU
             * HLT_OUT: reads as 0x00000000
             * @return CTSTW:
             * [31]    Error
             * [30:12] Command Parameter
             * [11:8]  Transaction ID
             * [7:5]   "000"
             * [4]     StartOfTransaction: The SOTR bit shall be set to ‘1’
             *         by the SIU, when a data block transmission
             *         transaction is opened by a FECMD
             * [3:2]   "00"
             * [1]     1 -> CTSTW coming from SIU
             * [0]     1 -> CTSTW coming from DIU
             * */
            uint32_t
            lastDiuCommandTransmissionStatusWord();


            /**
             * HLT_IN: clear last CommandTransmissionStatusWord.
             * after this call the CTSW register holds 0xffffffff
             * */
            void
            clearLastDiuCommandTransmissionStatusWord();


            /**
             * HLT_IN: get last DataTransmissionStatusWord from DIU
             * HLT_OUT: reads as 0x00000000 on SIU.
             * @return DTSTW
             * [31]    Error
             * [30:12] Block Length
             * [11:8]  Transaction ID
             * [7:0]   0x82
             * */
            uint32_t
            lastDiuDataTransmissionStatusWord();


            /**
             * HLT_IN: clear last DataTransmissionStatusWord.
             * after this call the DTSTW register holds 0xffffffff
             * */
            void
            clearLastDiuDataTransmissionStatusWord();


            /**
             * HLT_IN: get last InterfaceStatusWord from DIU,
             * reads as 0x00000000 on SIU.
             * @return IFSTW
             * [31]    Error
             * [30:12] Interface Status
             * [11:8]  Transaction ID
             * [7:2]   "110000"
             * [1]     1 -> IFSTW coming from SIU
             * [0]     1 -> IFSTW coming from DIU
             * */
            uint32_t
            lastDiuInterfaceStatusWord();


            /**
             * HLT_IN: clear last InterfaceStatusWord.
             * after this call the IFSTW register holds 0xffffffff
             * */
            void
            clearLastDiuInterfaceStatusWord();


            /**
             * HLT_IN: clear all last DIU status words
             * */
            void
            clearAllLastDiuStatusWords();


            /**
             * HLT_IN: wait for DIU StatusWord.
             * @param status value register address
             * @return 0xffffffff on timeout, else status value
             * */
            uint32_t
            waitForDiuStatusWord
            (
                uint32_t address
            );


            /**
             * HLT_IN: wait for CommandTransmissionStatusWord.
             * important: clear CTSW before calling this this function!
             * @return 0 on sucess, -1 on timeout
             * */
            int
            waitForDiuCommandTransmissionStatusWord();


            /**
             * HLT_IN: wait for InterfaceStatusWord.
             * important: clear CTSW before calling this this function!
             * @return 0 on sucess, -1 on timeout
             * */
            int
            waitForDiuInterfaceStatusWord();


            /**
             * get DDL Event Count: number of Events received from DIU
             * or sent via SIU.
             * This counter is implemented for both HLT_IN and HLT_OUT.
             * */
            uint32_t
            ddlEventCount();


            /**
             * get DDL deadtime as number of clock cycles.
             * HLT_IN:  number of clock cycles in which the RORC
             *          was sending XOFF. This counter should not
             *          increase.
             * HLT_OUT: number of clock cycles in which the SIU
             *          was throtteling data output. This counter
             *          is likely to increase.
             * @return number of clock cycles
             * */
            uint32_t
            ddlDeadtime();


            /**
             * clear DDL deadtime counter
             * */
            void
            clearDdlDeadtime();


            /**
             * HLT_IN: send command via DIU interface
             * @param command
             * */
            void
            diuSendCommand
            (
                uint32_t command
            );



            /**********************************************************
             *             Protocol Level DDL Status and Control
             * ********************************************************
             * these commands are sent via DIU command interface to
             * DIU, SIU or FEE.
             * NOTE: these commands only apply to HLT_IN.
             * See also http://cds.cern.ch/record/689275
             * *******************************************************/

            /**
             * HLT_IN: send ReadyToReceive command to FEE
             * @return 0 on sucess, -1 on error
             * */
            int
            sendFeeReadyToReceive();


            /**
             * HLT_IN: send EndOfBlockTransfer command to FEE
             * @return 0 on sucess, -1 on error
             * */
            int
            sendFeeEndOfBlockTransfer();


            /**
             * HLT_IN: read and clear DIU InterfaceStatusWord
             * @return IFSTW from DIU, see INT-1996-43, p.17/18
             * for bit encodings
             * */
            uint32_t
            readAndClearDiuInterfaceStatus();


            /**
             * HLT_IN: read and clear SIU InterfaceStatusWord
             * @return IFSTW from SIU, see INT-1996-43, p.17/18
             * for bit encodings
             * */
            uint32_t
            readAndClearSiuInterfaceStatus();


            /**
             * HLT_IN: send Link Reset Command to DIU.
             * The DIU shall go to off-line state when it receives
             * a LRST command from the RORC. The DIU shall activate
             * the riLD_N interface line, when it is staying in the
             * off-line state
             **/
            void
            sendDiuLinkReset();


            /**
             * HLT_IN: send Reset Command to SIU.
             * Start the reset cycle of the SIU. The SIU shall
             * automatically enter into the off-line state at the
             * end of the initialisation
             * */
            void
            sendSiuReset();


            /**
             * HLT_IN: send Link Initialization Command to DIU.
             * The DIU shall start the link initialisation protocol
             * when it receives a LINIT command from the RORC.
             * */
            void
            sendDiuLinkInitialization();


            /**
             * wait for link up, cleanly open FEE link
             **/
            void
            prepareDiuForFeeData();


            bool
            isDiuLinkUp();


            bool
            isDiuLinkFull();


            void
            enableDdl();


            /**********************************************************
             *             Data Path Configuration
             *********************************************************/

            /**
             * Set DIU as data source
             **/
            void
            setDataSourceDdl();

            /**
             * set DDR3 Data Replay as data source
             **/
            void
            setDataSourceDdr3DataReplay();

            /**********************************************************
             *             Fast Cluster Finder Interfacing
             *********************************************************/

            /**
             * load mapping file into FCF mapping RAM
             * @param fname path to mapping file. The mapping file
             *        is a plain-text file with one 32bit hex string
             *        per line, e.g.
             *        "0x1000801f"
             **/
            void
            fcfLoadMappingRam
            (
                 const char *fname
            );

            /**
             * enable FastClusterFinder processing
             **/
            void
            enableFcf();



            /**********************************************************
             *             Data Replay Channel Level Interfacing
             *********************************************************/

            /**
             * set start address of current channel's replay data
             * in DDR3 memory
             * @param ddr3_start_address replay data start address
             **/
            void
            configureDdr3DataReplayChannel
            (
                 uint32_t ddr3_start_address
            );

            /**
             * enable DDR3 data replay channel
             **/
            void
            enableDdr3DataReplayChannel();

            /**********************************************************
             *             Debug Output
             * *******************************************************/

            /**
             * Printout the DIU-state to the console
             * */
            void printDiuState();



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



            /**********************************************************
             *             Fast Cluster Finder Interfacing
             *********************************************************/

            /** write entry into FCF mapping RAM
             * @param addr mapping RAM addr
             * @param data data to be written
             **/
            void
            fcfWriteMappingRamEntry
            (
                 uint32_t addr,
                 uint32_t data
            );

            /**
             * convert hex string to uint32_t
             * @param line input string
             * @return line as uint32_t
             **/
            uint32_t
            fcfHexstringToUint32
            (
                 std::string line
            );

};

}

#endif /** LIBRORC_LINK_H */
