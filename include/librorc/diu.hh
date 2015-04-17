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

#ifndef LIBRORC_DIU_H
#define LIBRORC_DIU_H

#include <iostream>
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
#define LIBRORC_DIUCMD_REMOTE_HWVERS 0x00000062

namespace LIBRARY_NAME
{
    class link;

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
             * check if DDL link is full
             * @return true when full, false when not full
             **/
            bool linkFull();

            /**
             * DIU Interface internal counter providing the reported
             * event size (riD[31:12]) to DMA engine on each EndOfEvent.
             * Clearing this counter should only be required after a
             * crash. No doing so may result in in a mismatch of
             * calculated and reported event size. No data gets lost.
             * */
            void
            clearEventSizeCounter();

            uint32_t
            eventSizeCounter();

            /**
             * get num,ber of events received from DIU
             * @return number of events
             **/
            uint32_t
            getEventcount();

            /**
             * clear DIU event count
             **/
            void
            clearEventcount();


            /**
             * Clear all last DIU status words
             * */
            void
            clearAllLastStatusWords();

            /**
             * get DDL deadtime
             * @return deadtime in clock cycles. For HLT_IN this is the
             * number of cycles where XOFF was active -> should not
             * increase.
             **/
            uint32_t
            getDdlDeadtime();

            /**
             * clear DDL deadtime counter */
            void
            clearDdlDeadtime();

            uint32_t
            getForcedXoff();

            void
            setForcedXoff( uint32_t xoff );


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

            /**
             * Read from remote SIU EEPROM
             * @addr i2c eeprom address
             * @return FrontEndStatusWord on success or 0xffffffff on timeout
             * [31] I2C Error
             * [30] 0
             * [29] I2C Error
             * [28] 0
             * [27:20] I2C Data
             * [19:12] I2C Addr
             * [11:8] Transaction-ID
             * [7:4] "0110"
             * [3:0] Data Source: 0010 for remote SIU
             **/
            uint32_t
            readRemoteHwVersion
            (
                 uint8_t addr
            );

            /**
             * read full serial string from remote SIU.
             * @return string with link description and serial or empty string
             * on error.
             **/
            std::string
            readRemoteSerial();

            /**
             * get the last command sent via DIU interface
             * @return command word
             **/
            uint32_t
            lastDiuCommand();

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


        protected:
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


            /**
             * Blocking wait for FrontEndStatusWord.
             * important: clear FESTW before calling this this function!
             * @return FESTW on sucess, -1 on timeout
             * */
            int
            waitForFrontEndStatusWord();

            /**
             * set and clear DDL reset, wait for link to come up.
             * @return 0 on success, -1 on timeout waiting for link to come up
             **/
            int toggleDdlReset();
    };
}
#endif
