#include <librorc/registers.h>
#include <librorc/dma_channel.hh>
#include <librorc/dma_channel_ddl.hh>

#include <librorc/device.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/buffer.hh>

#include <pda.h>



namespace LIBRARY_NAME
{
    dma_channel_ddl::dma_channel_ddl
    (
        uint32_t  channel_number,
        uint32_t  pcie_packet_size,
        device   *dev,
        bar      *bar,
        buffer   *eventBuffer,
        buffer   *reportBuffer
    )
    : dma_channel(channel_number, pcie_packet_size, dev, bar, eventBuffer, reportBuffer)
    {
        enable();
        m_link->waitForGTXDomain();
        configureDDL();
    }

    dma_channel_ddl::~dma_channel_ddl()
    {
        closeDDL();
    }



    void
    dma_channel_ddl::configureDDL()
    {
        /** set ENABLE, activate flow control (DIU_IF:busy), MUX=0 */
        m_link->setGTX(RORC_REG_DDL_CTRL, 0x00000003);

        uint32_t timeout = LIBRORC_LINK_DDL_TIMEOUT;
        /** wait for riLD_N='1' */
        while( ((m_link->GTX(RORC_REG_DDL_CTRL) & 0x20) != 0x20) && (timeout!=0) )
        {
            timeout--;
            usleep(100);
        }

        if ( !timeout )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "Timeout waiting for LD_N to deassert");
        }
        else
        {
            /** clear DIU_IF IFSTW, CTSTW */
            m_link->setGTX(RORC_REG_DDL_IFSTW, 0);
            m_link->setGTX(RORC_REG_DDL_CTSTW, 0);

            /** send EOBTR to close any open transaction */
            m_link->setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

            m_link->waitForDiuCommandTransmissionStatusWord();

            /** clear DIU_IF IFSTW */
            m_link->setGTX(RORC_REG_DDL_IFSTW, 0);
            m_link->setGTX(RORC_REG_DDL_CTSTW, 0);

            /** send RdyRx to SIU */
            m_link->setGTX(RORC_REG_DDL_CMD, 0x00000014);

            m_link->waitForDiuCommandTransmissionStatusWord();
        }
    }



    void
    dma_channel_ddl::closeDDL()
    {
        /** check if link is still up: LD_N == 1 */
        if( m_link->GTX(RORC_REG_DDL_CTRL) & (1<<5) )
        {
            /** disable BUSY -> drop current data in chain */
            m_link->setGTX(RORC_REG_DDL_CTRL, 0x00000001);

            uint32_t timeout = LIBRORC_LINK_DDL_TIMEOUT;
            /** wait for LF_N to go high */
            while( (!(m_link->GTX(RORC_REG_DDL_CTRL) & (1<<4))) && (timeout!=0) )
            {
                usleep(100);
                timeout--;
            }

            if( !timeout )
            { DEBUG_PRINTF(PDADEBUG_ERROR, "Timeout waiting for LF_N to deassert\n"); }

            /** clear DIU_IF IFSTW */
            m_link->setGTX(RORC_REG_DDL_IFSTW, 0);
            m_link->setGTX(RORC_REG_DDL_CTSTW, 0);

            /** Send EOBTR command */
            m_link->setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

            /** wait for command transmission status word (CTST)
             * in response to the EOBTR:
             * STS[7:4]="0000"
             */
            timeout = LIBRORC_LINK_DDL_TIMEOUT;
            while( (m_link->GTX(RORC_REG_DDL_CTSTW) & 0xf0) && (timeout!=0) )
            {
                usleep(100);
                timeout--;
            }

            if( !timeout )
            { DEBUG_PRINTF(PDADEBUG_ERROR, "Timeout waiting for CTSTW\n"); }

            /** disable DIU_IF */
            m_link->setGTX(RORC_REG_DDL_CTRL, 0x00000000);
        }
        else
        { throw LIBRORC_DMA_CHANNEL_ERROR_CLOSE_GTX_FAILED; }
    }
}
