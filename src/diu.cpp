/**
 * @file diu.cpp
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

#include <librorc/diu.hh>

namespace LIBRARY_NAME
{
    diu::diu( link *link ) : ddl(link)
    {
        m_link = link;
    }


    diu::~diu()
    {
        m_link = NULL;
    }


    bool
    diu::linkUp()
    {
        return ( (m_link->GTX(RORC_REG_DDL_CTRL) & (1<<5)) != 0 );
    }


    void
    diu::clearEventSizeCounter()
    {
        m_link->setGTX(RORC_REG_DDL_CTRL,
                m_link->GTX(RORC_REG_DDL_CTRL)|(1<<2));
    }


    void
    diu::clearAllLastStatusWords()
    {
        clearLastFrontEndStatusWord();
        clearLastCommandTransmissionStatusWord();
        clearLastDataTransmissionStatusWord();
        clearLastInterfaceStatusWord();
    }


    /**********************************************************
     *             Protocol Level DDL Status and Control
     * *******************************************************/
    int
    diu::sendFeeEndOfBlockTransferCmd()
    {
        clearLastCommandTransmissionStatusWord();
        sendCommand(LIBRORC_DIUCMD_FEE_EOBTR); // EOBTR to FEE
        return waitForCommandTransmissionStatusWord();
    }


    int
    diu::sendFeeReadyToReceiveCmd()
    {
        clearLastCommandTransmissionStatusWord();
        sendCommand(LIBRORC_DIUCMD_FEE_RDYRX); // RdyRx to FEE
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
        sendCommand(LIBRORC_DIUCMD_SIU_RST); // SRST to SIU
        return waitForInterfaceStatusWord();
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


    bool
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
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "Timeout waiting for LF_N to deassert\n");
        }

        return (timeout!=0);
    }


    /**
     * TODO: should this throw/return an error instead of just printing
     * to PDA_DEBUG?
     * PRO: it is important to know if data can be received at all or
     * if further action is required
     * CON: DIU/SIU state is independent of any DMA/readout channel
     * and can be changed/controlled while the readout is running. Is
     * there anything checking the link state periodically?
     **/
    void
    diu::prepareForFeeData()
    {
        /** check link state before sending commands */
        if( !m_link->isGtxDomainReady() )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Unexpected GTX state -"
                    " will not send DIU commands!\n");
        }
        else
        {
            /** reset DIU */
            setReset(1);
            setReset(0);
            waitForLinkUp();

            /** close any open SIU transfers */
            sendFeeEndOfBlockTransferCmd();
            /** reset the SIU */
            //sendSiuResetCmd();

            /** reset DIU again */
            //setReset(1);
            //setReset(0);
            //waitForLinkUp();

            /** re-open the link */
            sendFeeReadyToReceiveCmd();
        }
    }


    void
    diu::useAsDataSource()
    {
        uint32_t ddlctrl = m_link->GTX(RORC_REG_DDL_CTRL);
        ddlctrl &= ~(3<<16); // set MUX to 0
        m_link->setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }


    /**********************************************************
     *                  protected
     * *******************************************************/
    void
    diu::sendCommand
    (
        uint32_t command
    )
    {
        m_link->setGTX(RORC_REG_DDL_CMD, command);
    }    uint32_t
    diu::lastFrontEndStatusWord()
    {
        return m_link->GTX(RORC_REG_DDL_FESTW);
    }

    void
    diu::clearLastFrontEndStatusWord()
    {
        m_link->setGTX(RORC_REG_DDL_FESTW, 0);
    }


    uint32_t
    diu::lastCommandTransmissionStatusWord()
    {
        return m_link->GTX(RORC_REG_DDL_CTSTW);
    }

    void
    diu::clearLastCommandTransmissionStatusWord()
    {
        m_link->setGTX(RORC_REG_DDL_CTSTW, 0);
    }


    uint32_t
    diu::lastDataTransmissionStatusWord()
    {
        return m_link->GTX(RORC_REG_DDL_DTSTW);
    }

    void
    diu::clearLastDataTransmissionStatusWord()
    {
        m_link->setGTX(RORC_REG_DDL_DTSTW, 0);
    }


    uint32_t
    diu::lastInterfaceStatusWord()
    {
        return m_link->GTX(RORC_REG_DDL_IFSTW);
    }

    void
    diu::clearLastInterfaceStatusWord()
    {
        m_link->setGTX(RORC_REG_DDL_IFSTW, 0);
    }


    uint32_t
    diu::waitForStatusWord
    (
        uint32_t address
    )
    {
        uint32_t timeout = LIBRORC_DIU_TIMEOUT;
        uint32_t status;

        while( ((status = m_link->GTX(address)) == 0xffffffff) &&
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
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "Timeout waiting for DIU "
                    "CommandTransmissionStatusWord\n");
            return -1;
        }
        else
        {
            DEBUG_PRINTF(PDADEBUG_VALUE,
                    "waitForCommandTransmissionStatusWord: "
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
    diu::waitForInterfaceStatusWord()
    {
        uint32_t result = waitForStatusWord(RORC_REG_DDL_IFSTW);

        if (result==0xffffffff)
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "Timeout waiting for DDL "
                    "InterfaceStatusWord\n");
        }
        else
        {
            DEBUG_PRINTF(PDADEBUG_VALUE,
                    "waitForInterfaceStatusWord: "
                    "got IFSTW: %08x\n", result);
        }
        return result;
    }
}
