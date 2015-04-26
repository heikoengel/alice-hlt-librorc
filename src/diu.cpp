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

#include <unistd.h>

#include <librorc/diu.hh>
#include <librorc/registers.h>
#include <librorc/link.hh>

namespace LIBRARY_NAME
{

    diu::diu( link *link ) : ddl(link){}
    diu::~diu(){}

    bool
    diu::linkUp()
    {
        return ( (m_link->ddlReg(RORC_REG_DDL_CTRL) & (1<<5)) != 0 );
    }

    bool
    diu::linkFull()
    { return ( ((m_link->ddlReg(RORC_REG_DDL_CTRL)>>4) & 1) != 1 ); }

    uint32_t
    diu::getForcedXoff()
    { return ((m_link->ddlReg(RORC_REG_DDL_CTRL) >> 6) & 1); }

    void
    diu::setForcedXoff( uint32_t xoff ) {
        std::cout << "Setting XOFF to " << xoff << std::endl;
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(1<<6);
        ddlctrl |= ((xoff&1) << 6);
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }

    void
    diu::clearEventSizeCounter()
    {
        m_link->setDdlReg(RORC_REG_DDL_CTRL,
                m_link->ddlReg(RORC_REG_DDL_CTRL)|(1<<2));
    }

    uint32_t
    diu::totalWordsReceived()
    {
        return m_link->ddlReg(RORC_REG_DDL_TOTAL_WORDS);
    }

    void
    diu::clearAllLastStatusWords()
    {
        clearLastFrontEndStatusWord();
        clearLastCommandTransmissionStatusWord();
        clearLastDataTransmissionStatusWord();
        clearLastInterfaceStatusWord();
    }

    uint32_t
    diu::getDdlDeadtime()
    { return m_link->ddlReg(RORC_REG_DDL_DEADTIME); }

    void
    diu::clearDdlDeadtime()
    { m_link->setDdlReg(RORC_REG_DDL_DEADTIME, 0); }

    uint32_t
    diu::getEventcount()
    { return m_link->ddlReg(RORC_REG_DDL_EC); }

    void
    diu::clearEventcount()
    { m_link->setDdlReg(RORC_REG_DDL_EC, 0); }

    /**********************************************************
     *             Protocol Level  DDL Status and Control
     * *******************************************************/
    int
    diu::sendFeeEndOfBlockTransferCmd()
    {
        clearLastCommandTransmissionStatusWord();
        sendCommand(LIBRORC_DIUCMD_SIU_EOBTR); // EOBTR to SIU
        return waitForCommandTransmissionStatusWord();
    }


    int
    diu::sendFeeReadyToReceiveCmd()
    {
        clearLastCommandTransmissionStatusWord();
        sendCommand(LIBRORC_DIUCMD_SIU_RDYRX); // RdyRx to SIU
        return waitForCommandTransmissionStatusWord();
    }


    uint32_t
    diu::readAndClearDiuInterfaceStatus()
    {
        clearLastInterfaceStatusWord();
        sendCommand(LIBRORC_DIUCMD_DIU_RCIFSTW); // R&CIFSTW to DIU
        return waitForInterfaceStatusWord();
    }


    uint32_t
    diu::readAndClearSiuInterfaceStatus()
    {
        clearLastInterfaceStatusWord();
        sendCommand(LIBRORC_DIUCMD_SIU_RCIFSTW); // R&CIFSTW to SIU
        return waitForInterfaceStatusWord();
    }


    uint32_t
    diu::sendSiuResetCmd()
    {
        clearLastInterfaceStatusWord();
        sendCommand(LIBRORC_DIUCMD_SIU_RST); // SIU-RESET to DIU
        return waitForCommandTransmissionStatusWord();
    }


    uint32_t
    diu::sendDiuLinkInitializationCmd()
    {
        clearLastInterfaceStatusWord();
        sendCommand(LIBRORC_DIUCMD_DIU_DIUINIT); // LINIT to DIU
        return waitForInterfaceStatusWord();
    }


    uint32_t
    diu::sendDiuLinkResetCmd()
    {
        clearLastInterfaceStatusWord();
        sendCommand(LIBRORC_DIUCMD_DIU_DIURST); // SRST to DIU
        return waitForInterfaceStatusWord();
    }


    int
    diu::waitForLinkUp()
    {
        uint32_t timeout = LIBRORC_DIU_TIMEOUT;
        /** wait for LF_N to go high */
        while( !linkUp() && (timeout!=0) )
        {
            usleep(100);
            timeout--;
        }

        if ( !timeout )
        { return -1; }
        else
        { return 0; }
    }

    int
    diu::prepareForSiuData()
    {
        if( toggleDdlReset() < 0 )
        { return -1; }
        if( sendSiuResetCmd() < 0 )
        { return -1; }
        if( toggleDdlReset() < 0 )
        { return -1; }
        return 0;
    }

    int
    diu::prepareForDiuData()
    {
        return toggleDdlReset();
    }

    int
    diu::toggleDdlReset()
    {
        setReset(1);
        getReset();
        setReset(0);
        getReset();
        return waitForLinkUp();
    }


    void
    diu::useAsDataSource()
    {
        uint32_t ddlctrl = m_link->ddlReg(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(3<<16); // set MUX to 0
        m_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
    }


    uint32_t
    diu::readRemoteHwVersion
    (
       uint8_t addr
    )
    {
        clearLastFrontEndStatusWord();
        uint32_t cmd = LIBRORC_DIUCMD_REMOTE_HWVERS;
        cmd |= ((uint32_t)addr)<<12;
        sendCommand(cmd);
        return waitForFrontEndStatusWord();
    }


    std::string
    diu::readRemoteSerial()
    {
        std::string serial = std::string();
        for( uint32_t addr=0; addr<50; addr++ )
        {
            uint32_t result = readRemoteHwVersion( addr );
            if( result!=0xffffffff )
            { serial.append( 1, ((result>>20)&0xff) ); }
            else
            {  break; }
        }
        return serial;
    }

    uint32_t
    diu::lastDiuCommand()
    {
        return m_link->ddlReg(RORC_REG_DDL_CMD);
    }

    void
    diu::sendCommand
    (
        uint32_t command
    )
    {
        m_link->setDdlReg(RORC_REG_DDL_CMD, command);
    }

    uint32_t
    diu::lastFrontEndStatusWord()
    {
        return m_link->ddlReg(RORC_REG_DDL_FESTW);
    }

    void
    diu::clearLastFrontEndStatusWord()
    {
        m_link->setDdlReg(RORC_REG_DDL_FESTW, 0);
    }


    uint32_t
    diu::lastCommandTransmissionStatusWord()
    {
        return m_link->ddlReg(RORC_REG_DDL_CTSTW);
    }

    void
    diu::clearLastCommandTransmissionStatusWord()
    {
        m_link->setDdlReg(RORC_REG_DDL_CTSTW, 0);
    }


    uint32_t
    diu::lastDataTransmissionStatusWord()
    {
        return m_link->ddlReg(RORC_REG_DDL_DTSTW);
    }

    void
    diu::clearLastDataTransmissionStatusWord()
    {
        m_link->setDdlReg(RORC_REG_DDL_DTSTW, 0);
    }


    uint32_t
    diu::lastInterfaceStatusWord()
    {
        return m_link->ddlReg(RORC_REG_DDL_IFSTW);
    }

    void
    diu::clearLastInterfaceStatusWord()
    {
        m_link->setDdlReg(RORC_REG_DDL_IFSTW, 0);
    }


    /**********************************************************
     *                  protected
     * *******************************************************/
    uint32_t
    diu::waitForStatusWord
    (
        uint32_t address
    )
    {
        uint32_t timeout = LIBRORC_DIU_TIMEOUT;
        uint32_t status;

        while( ((status = m_link->ddlReg(address)) == 0xffffffff) &&
                (timeout!=0) )
        {
            usleep(100);
            timeout--;
        }

        /** return 0xffffffff on timeout, value on sucess */
        return status;
    }

    int
    diu::waitForCommandTransmissionStatusWord()
    {
        uint32_t result = waitForStatusWord(RORC_REG_DDL_CTSTW);

        if(result==0xffffffff)
        { return -1; }
        else
        { return 0; }

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
    diu::waitForInterfaceStatusWord()
    {
        uint32_t result = waitForStatusWord(RORC_REG_DDL_IFSTW);
        return result;
    }


    int
    diu::waitForFrontEndStatusWord()
    {
        uint32_t result = waitForStatusWord(RORC_REG_DDL_FESTW);
        return result;
    }
}
