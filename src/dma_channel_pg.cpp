#include <librorc/registers.h>
#include <librorc/dma_channel.hh>
#include <librorc/dma_channel_pg.hh>

#include <librorc/device.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/buffer.hh>

#include <pda.h>



namespace LIBRARY_NAME
{
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
        enable();
        waitForGTXDomain();
        configurePatternGenerator(eventSize);
    }

    dma_channel_pg::~dma_channel_pg()
    {
        closePatternGenerator();
    }



    void
    dma_channel_pg::configurePatternGenerator(uint32_t eventSize)
    {
        /** Configure Pattern Generator */
        setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, eventSize);
        uint32_t ddlctrl = (1<<0) | //enable DDLIF
            (1<<1) | // enable flow control
            (1<<3) | // set MUX to use PG as data source
            (1<<9) | // enable PG adaptive
            (1<<10) | // enable continuous mode
            (1<<8) | // enable PG
            (1<<13) | // enable PRBS EventLength
            (0<<11); // set mode to INCREMENT
        setGTX(RORC_REG_DDL_CTRL, ddlctrl);
    }



    void
    dma_channel_pg::closePatternGenerator()
    {
        setGTX(RORC_REG_DDL_CTRL, 0x0);
    }

}
