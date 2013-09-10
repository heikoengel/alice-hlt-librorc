#include <librorc/registers.h>
#include <librorc/dma_channel.hh>
#include <librorc/dma_channel_ddl.hh>

#include <librorc/device.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/buffer.hh>

#include <pda.h>

using namespace std;

/** extern error number **/
extern int errno;



namespace LIBRARY_NAME
{

    dma_channel_ddl::dma_channel_ddl
    (
        uint32_t  channel_number,
        device   *dev,
        bar      *bar
    )
    : dma_channel(channel_number, dev, bar)
    {

    }



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
        //cout << "Enabling DDL link!" << endl;
        enable();
        //cout << "Waiting for GTX to be ready..." << endl;
        waitForGTXDomain();
        //cout << "Configuring GTX ..." << endl;
        configureDDL();
    }



    dma_channel_ddl::~dma_channel_ddl()
    {
        closeDDL();
        disable();

        if(m_reportBuffer != NULL)
        {
            m_reportBuffer->clear();
        }
    }



    void
    dma_channel_ddl::configureDDL()
    {
        /** set ENABLE, activate flow control (DIU_IF:busy) */
        setGTX(RORC_REG_DDL_CTRL, 0x00000003);

        /** wait for riLD_N='1' */
        while( (getGTX(RORC_REG_DDL_CTRL) & 0x20) != 0x20 )
            {usleep(100);}

        /** clear DIU_IF IFSTW, CTSTW */
        setGTX(RORC_REG_DDL_IFSTW, 0);
        setGTX(RORC_REG_DDL_CTSTW, 0);

        /** send EOBTR to close any open transaction */
        setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

        waitForCommandTransmissionStatusWord();

        /** clear DIU_IF IFSTW */
        setGTX(RORC_REG_DDL_IFSTW, 0);
        setGTX(RORC_REG_DDL_CTSTW, 0);

        /** send RdyRx to SIU */
        setGTX(RORC_REG_DDL_CMD, 0x00000014);

        waitForCommandTransmissionStatusWord();

        /** clear DIU_IF IFSTW */
        setGTX(RORC_REG_DDL_IFSTW, 0);
        setGTX(RORC_REG_DDL_CTSTW, 0);
    }



    void
    dma_channel_ddl::closeDDL()
    {
        /** check if link is still up: LD_N == 1 */
        if( getGTX(RORC_REG_DDL_CTRL) & (1<<5) )
        {
            /** disable BUSY -> drop current data in chain */
            setGTX(RORC_REG_DDL_CTRL, 0x00000001);

            /** wait for LF_N to go high */
            while(!(getGTX(RORC_REG_DDL_CTRL) & (1<<4)))
            {usleep(100);}

            /** clear DIU_IF IFSTW */
            setGTX(RORC_REG_DDL_IFSTW, 0);
            setGTX(RORC_REG_DDL_CTSTW, 0);

            /** Send EOBTR command */
            setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

            /** wait for command transmission status word (CTST)
             * in response to the EOBTR:
             * STS[7:4]="0000"
             */
            while(getGTX(RORC_REG_DDL_CTSTW) & 0xf0)
            {usleep(100);}

            /** disable DIU_IF */
            setGTX(RORC_REG_DDL_CTRL, 0x00000000);
        }
        else
        { throw LIBRORC_DMA_CHANNEL_ERROR_CLOSE_GTX_FAILED; }
    }
}
