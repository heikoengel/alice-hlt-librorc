#include <librorc/registers.h>
#include <librorc/dma_channel.hh>
#include <librorc/dma_channel_pg.hh>

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

    dma_channel_pg::dma_channel_pg
    (
        uint32_t  channel_number,
        device   *dev,
        bar      *bar
    )
    : dma_channel(channel_number, dev, bar)
    {

    }

    dma_channel_pg::dma_channel_pg
    (
        uint32_t  channel_number,
        uint32_t  pcie_packet_size,
        device   *dev,
        bar      *bar,
        buffer   *eventBuffer,
        buffer   *reportBuffer,
        uint32_t  eventSize
    )
    : dma_channel(channel_number, pcie_packet_size, dev, bar, eventBuffer, reportBuffer)
    {
        //cout << "Enabling Pattern Generator!" << endl;
        enable();
        //cout << "Waiting for GTX to be ready..." << endl;
        waitForGTXDomain();
        //cout << "Configuring pattern generator ..." << endl;
        configurePatternGenerator(eventSize);
    }

    dma_channel_pg::~dma_channel_pg()
    {
        closePatternGenerator();
        disable();

        if(m_reportBuffer != NULL)
        {
            m_reportBuffer->clear();
        }
    }



    void
    dma_channel_pg::configurePatternGenerator(uint32_t eventSize)
    {
        if(m_is_gtx)
        {
            throw LIBRORC_DMA_CHANNEL_ERROR_ENABLE_PATTERN_GENERATOR_FAILED;
        }

        /** Configure Pattern Generator */
        setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, eventSize);
        setGTX(RORC_REG_DDL_CTRL, (getGTX(RORC_REG_DDL_CTRL) | 0x600) );
        setGTX(RORC_REG_DDL_CTRL, (getGTX(RORC_REG_DDL_CTRL) | 0x100) );
        m_is_pattern_generator = true;
    }



    void
    dma_channel_pg::closePatternGenerator()
    {
        if(!m_is_pattern_generator)
        {
            throw LIBRORC_DMA_CHANNEL_ERROR_CLOSE_PATTERN_GENERATOR_FAILED;
        }

        setGTX(RORC_REG_DDL_CTRL, 0x0);

        m_is_pattern_generator = false;
    }

}