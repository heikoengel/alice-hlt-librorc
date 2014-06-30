/**
 * @file diu.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-14
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
 * */

#ifndef LIBRORC_DIU_H
#define LIBRORC_DIU_H

#include <librorc/defines.hh>
#include <librorc/ddl.hh>

#define LIBRORC_DIU_TIMEOUT 10000

/** commands to be sent via DIU, naming scheme:
 * LIBRORC_DIUCMD_[destination]_[cmd]
 **/
#define LIBRORC_DIUCMD_DIU_RCIFSTW 0x00000001
#define LIBRORC_DIUCMD_SIU_RCIFSTW 0x00000002
#define LIBRORC_DIUCMD_SIU_EOBTR 0x000000b4
#define LIBRORC_DIUCMD_SIU_RDYRX 0x00000014
#define LIBRORC_DIUCMD_DIU_DIURST 0x000000a1
#define LIBRORC_DIUCMD_SIU_RST 0x000000f1
#define LIBRORC_DIUCMD_DIU_DIUINIT 0x000000b1

namespace LIBRARY_NAME
{
    class link;
    class ddl;

    /**
     * @brief Destination Interface Unit (DIU) control class
     **/
    class diu : public ddl
    {
        public:
            diu( link *link );
            ~diu();


            /**
             * check if DIU link is up
             * @return true when up, false when down
             **/
            bool
            linkUp();


            /**
             * DIU Interface internal counter providing the reported
             * event size (riD[31:12]) to DMA engine on each EndOfEvent.
             * Clearing this counter should only be required after a
             * crash. No doing so may result in in a mismatch of
             * calculated and reported event size. No data gets lost.
             * */
            void
            clearEventSizeCounter();


            /**
             * Clear all last DIU status words
             * */
            void
            clearAllLastStatusWords();



            /**********************************************************
             *             Protocol Level DDL Status and Control
             * ********************************************************
             * these commands are sent via DIU command interface to
             * DIU, SIU or FEE.
             * See also http://cds.cern.ch/record/689275
             * *******************************************************/

            /**
             * Send ReadyToReceive command to FEE
             * @return 0 on sucess, -1 on error
             * */
            int
            sendFeeReadyToReceiveCmd();


            /**
             * Send EndOfBlockTransfer command to FEE
             * @return 0 on sucess, -1 on error
             * */
            int
            sendFeeEndOfBlockTransferCmd();


            /**
             * Read and clear DIU InterfaceStatusWord
             * @return IFSTW from DIU, see INT-1996-43, p.17/18
             * for bit encodings
             * */
            uint32_t
            readAndClearDiuInterfaceStatus();


            /**
             * Read and clear SIU InterfaceStatusWord
             * @return IFSTW from SIU, see INT-1996-43, p.17/18
             * for bit encodings
             * */
            uint32_t
            readAndClearSiuInterfaceStatus();


            /**
             * Send Link Reset Command to DIU.
             * The DIU shall go to off-line state when it receives
             * a LRST command from the RORC. The DIU shall activate
             * the riLD_N interface line, when it is staying in the
             * off-line state
             * @return interface status word
             **/
            uint32_t
            sendDiuLinkResetCmd();


            /**
             * Send Reset Command to SIU.
             * Start the reset cycle of the SIU. The SIU shall
             * automatically enter into the off-line state at the
             * end of the initialisation
             * @return interface status word
             * */
            uint32_t
            sendSiuResetCmd();


            /**
             * Send Link Initialization Command to DIU.
             * The DIU shall start the link initialisation protocol
             * when it receives a LINIT command from the RORC.
             * @return interface status word
             * */
            uint32_t
            sendDiuLinkInitializationCmd();


            /**
             * Prepare DIU for data from SIU, includes sending
             * RDYRX to SIU
             * @return 0 on success, -1 on error
             **/
            int
            prepareForSiuData();

            /**
             * prepare DIU for data from another DIU
             * @return 0 on success, -1 on error
             **/
            int
            prepareForDiuData();

            /**
             * configure DIU to be used as data source
             **/
            void
            useAsDataSource();

            /**
             * Blocking wait for LD_N='1'
             * @return 0 on success, -1 on error
             **/
            int
            waitForLinkUp();


        protected:
            link *m_link;

            /**
             * HLT_IN: wait for DIU StatusWord. This is called by
             * waitFor***StatusWord().
             * @param status value register address
             * @return 0xffffffff on timeout, else status value
             * */
            uint32_t
            waitForStatusWord
            (
                uint32_t address
            );


            /**
             * Send command via DIU interface
             * @param command
             * */
            void
            sendCommand
            (
                uint32_t command
            );           /**
             * get last FrontEndStatusWord received on DIU
             * @return FESTW:
             * [31]    Error
             * [30:12] Front-End Status
             * [11:8]  Transaction ID
             * [7:6]   "01"
             * [5]     End Of Data Block (EODB)
             * [4:0]   "00100"
             * if FESTW is 0xffffffff the status word has not yet been
             * by the DIU.
             * */
            uint32_t
            lastFrontEndStatusWord();


            /**
             * clear last CommandTransmissionStatusWord.
             * after this call the register holds 0xffffffff. Do this before
             * requesting a new FESTW.
             * */
            void
            clearLastFrontEndStatusWord();


            /**
             * Get last CommandTransmissionStatusWord from DIU
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
            lastCommandTransmissionStatusWord();


            /**
             * Clear last CommandTransmissionStatusWord.
             * after this call the CTSW register holds 0xffffffff
             * */
            void
            clearLastCommandTransmissionStatusWord();


            /**
             * Get last DataTransmissionStatusWord from DIU
             * @return DTSTW
             * [31]    Error
             * [30:12] Block Length
             * [11:8]  Transaction ID
             * [7:0]   0x82
             * */
            uint32_t
            lastDataTransmissionStatusWord();


            /**
             * Clear last DataTransmissionStatusWord.
             * after this call the DTSTW register holds 0xffffffff
             * */
            void
            clearLastDataTransmissionStatusWord();


            /**
             * Get last InterfaceStatusWord from DIU,
             * @return IFSTW
             * [31]    Error
             * [30:12] Interface Status
             * [11:8]  Transaction ID
             * [7:2]   "110000"
             * [1]     1 -> IFSTW coming from SIU
             * [0]     1 -> IFSTW coming from DIU
             * */
            uint32_t
            lastInterfaceStatusWord();


            /**
             * Clear last InterfaceStatusWord.
             * after this call the IFSTW register holds 0xffffffff
             * */
            void
            clearLastInterfaceStatusWord();


            /**
             * Blocking wait for CommandTransmissionStatusWord.
             * important: clear CTSW before calling this this function!
             * @return 0 on sucess, -1 on timeout
             * */
            int
            waitForCommandTransmissionStatusWord();


            /**
             * Blocking wait for InterfaceStatusWord.
             * important: clear CTSW before calling this this function!
             * @return 0 on sucess, -1 on timeout
             * */
            int
            waitForInterfaceStatusWord();
    };
}
#endif
